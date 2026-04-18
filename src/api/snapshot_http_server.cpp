#include "api/snapshot_http_server.h"

#include "common/audio_state.h"
#include "state/audio_state_store.h"

#include <QFile>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>

namespace plasma_bridge::api
{
namespace
{

constexpr qsizetype kMaxRequestBytes = 16 * 1024;
const QByteArray kHeaderDelimiter = QByteArrayLiteral("\r\n\r\n");
const QString kSinksPath = QStringLiteral("/snapshot/audio/sinks");
const QString kDefaultSinkPath = QStringLiteral("/snapshot/audio/default-sink");
const QString kDocsRootPath = QStringLiteral("/docs/");
const QString kDocsRootNoSlashPath = QStringLiteral("/docs");
const QString kDocsHttpPath = QStringLiteral("/docs/http");
const QString kDocsWsPath = QStringLiteral("/docs/ws");
const QString kDocsOpenApiPath = QStringLiteral("/docs/openapi.yaml");
const QString kDocsAsyncApiPath = QStringLiteral("/docs/asyncapi.yaml");
const QString kDocsNoticesPath = QStringLiteral("/docs/assets/THIRD_PARTY_NOTICES.txt");
const QString kDefaultOpenApiServerUrl = QStringLiteral("http://127.0.0.1:8080");
const QString kDefaultAsyncApiHost = QStringLiteral("127.0.0.1:8081");

QString urlHost(const QString &host)
{
    return host.contains(QLatin1Char(':')) && !host.startsWith(QLatin1Char('[')) ? QStringLiteral("[%1]").arg(host) : host;
}

QString resourcePathForDocumentationAsset(const QString &path)
{
    if (path == kDocsRootPath) {
        return QStringLiteral(":/docs/index.html");
    }
    if (path == kDocsHttpPath) {
        return QStringLiteral(":/docs/http.html");
    }
    if (path == kDocsWsPath) {
        return QStringLiteral(":/docs/ws.html");
    }
    if (path == kDocsNoticesPath) {
        return QStringLiteral(":/docs/assets/THIRD_PARTY_NOTICES.txt");
    }
    if (path.startsWith(QStringLiteral("/docs/assets/"))) {
        return QStringLiteral(":%1").arg(path);
    }
    return {};
}

QByteArray contentTypeForPath(const QString &path)
{
    if (path.endsWith(QStringLiteral(".html"))) {
        return QByteArrayLiteral("text/html; charset=utf-8");
    }
    if (path.endsWith(QStringLiteral(".css"))) {
        return QByteArrayLiteral("text/css; charset=utf-8");
    }
    if (path.endsWith(QStringLiteral(".js"))) {
        return QByteArrayLiteral("application/javascript; charset=utf-8");
    }
    if (path.endsWith(QStringLiteral(".yaml"))) {
        return QByteArrayLiteral("application/yaml; charset=utf-8");
    }
    return QByteArrayLiteral("text/plain; charset=utf-8");
}

QByteArray readResourceBytes(const QString &resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    return file.readAll();
}

QString requestPathFromTarget(const QByteArray &target)
{
    QString path = QString::fromUtf8(target);
    const qsizetype queryIndex = path.indexOf(QLatin1Char('?'));
    if (queryIndex >= 0) {
        path.truncate(queryIndex);
    }

    return path;
}

QByteArray buildHttpResponse(int statusCode,
                             const QByteArray &reasonPhrase,
                             const QByteArray &contentType,
                             const QByteArray &body,
                             const QList<QPair<QByteArray, QByteArray>> &extraHeaders)
{
    QByteArray response;
    response += "HTTP/1.1 " + QByteArray::number(statusCode) + ' ' + reasonPhrase + "\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    response += "Connection: close\r\n";

    for (const auto &header : extraHeaders) {
        response += header.first + ": " + header.second + "\r\n";
    }

    response += "\r\n";
    response += body;
    return response;
}

} // namespace

SnapshotHttpServer::SnapshotHttpServer(state::AudioStateStore *audioStateStore,
                                       const QString &documentationHost,
                                       quint16 documentationHttpPort,
                                       quint16 documentationWsPort,
                                       QObject *parent)
    : QObject(parent)
    , m_audioStateStore(audioStateStore)
    , m_documentationHost(documentationHost)
    , m_documentationHttpPort(documentationHttpPort)
    , m_documentationWsPort(documentationWsPort)
{
    connect(&m_server, &QTcpServer::newConnection, this, &SnapshotHttpServer::handleNewConnection);
}

bool SnapshotHttpServer::listen(const QHostAddress &address, quint16 port)
{
    return m_server.listen(address, port);
}

QString SnapshotHttpServer::errorString() const
{
    return m_server.errorString();
}

QHostAddress SnapshotHttpServer::serverAddress() const
{
    return m_server.serverAddress();
}

quint16 SnapshotHttpServer::serverPort() const
{
    return m_server.serverPort();
}

void SnapshotHttpServer::handleNewConnection()
{
    while (QTcpSocket *socket = m_server.nextPendingConnection()) {
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            handleReadyRead(socket);
        });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            m_pendingRequests.remove(socket);
            socket->deleteLater();
        });
    }
}

void SnapshotHttpServer::handleReadyRead(QTcpSocket *socket)
{
    if (socket == nullptr) {
        return;
    }

    QByteArray &buffer = m_pendingRequests[socket];
    buffer += socket->readAll();

    if (buffer.size() > kMaxRequestBytes) {
        writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                               QStringLiteral("Request is too large."));
        m_pendingRequests.remove(socket);
        return;
    }

    const qsizetype headerEnd = buffer.indexOf(kHeaderDelimiter);
    if (headerEnd < 0) {
        return;
    }

    const QByteArray requestData = buffer.left(headerEnd);
    m_pendingRequests.remove(socket);
    processRequest(socket, requestData);
}

void SnapshotHttpServer::processRequest(QTcpSocket *socket, const QByteArray &requestData)
{
    const QList<QByteArray> lines = requestData.split('\n');
    if (lines.isEmpty()) {
        writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                               QStringLiteral("Malformed HTTP request."));
        return;
    }

    const QList<QByteArray> requestLineParts = lines.first().trimmed().split(' ');
    if (requestLineParts.size() != 3) {
        writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                               QStringLiteral("Malformed HTTP request line."));
        return;
    }

    const QByteArray method = requestLineParts.at(0);
    const QString path = requestPathFromTarget(requestLineParts.at(1));

    if (path == kDocsRootNoSlashPath) {
        writeByteResponse(socket,
                          301,
                          QByteArrayLiteral("Moved Permanently"),
                          QByteArrayLiteral("text/plain; charset=utf-8"),
                          QByteArrayLiteral("Redirecting to /docs/."),
                          {{QByteArrayLiteral("Location"), QByteArrayLiteral("/docs/")}});
        return;
    }

    const QString documentationResourcePath = resourcePathForDocumentationAsset(path);
    if (!documentationResourcePath.isEmpty() || path == kDocsOpenApiPath || path == kDocsAsyncApiPath) {
        if (method != QByteArrayLiteral("GET")) {
            writeJsonErrorResponse(socket,
                                   405,
                                   QByteArrayLiteral("Method Not Allowed"),
                                   QStringLiteral("method_not_allowed"),
                                   QStringLiteral("Only GET is supported for this endpoint."),
                                   {{QByteArrayLiteral("Allow"), QByteArrayLiteral("GET")}});
            return;
        }

        if (path == kDocsOpenApiPath) {
            QByteArray body = readResourceBytes(QStringLiteral(":/docs/specs/openapi.yaml"));
            body.replace(kDefaultOpenApiServerUrl.toUtf8(),
                         QStringLiteral("http://%1:%2").arg(urlHost(m_documentationHost),
                                                             QString::number(m_documentationHttpPort)).toUtf8());
            writeByteResponse(socket, 200, QByteArrayLiteral("OK"), contentTypeForPath(path), body);
            return;
        }

        if (path == kDocsAsyncApiPath) {
            QByteArray body = readResourceBytes(QStringLiteral(":/docs/specs/asyncapi.yaml"));
            body.replace(kDefaultAsyncApiHost.toUtf8(),
                         QStringLiteral("%1:%2").arg(urlHost(m_documentationHost),
                                                     QString::number(m_documentationWsPort)).toUtf8());
            writeByteResponse(socket, 200, QByteArrayLiteral("OK"), contentTypeForPath(path), body);
            return;
        }

        const QByteArray body = readResourceBytes(documentationResourcePath);
        if (body.isEmpty()) {
            writeJsonErrorResponse(socket, 404, QByteArrayLiteral("Not Found"), QStringLiteral("not_found"),
                                   QStringLiteral("Unknown path."));
            return;
        }

        writeByteResponse(socket, 200, QByteArrayLiteral("OK"), contentTypeForPath(documentationResourcePath), body);
        return;
    }

    const bool knownSnapshotPath = path == kSinksPath || path == kDefaultSinkPath;
    if (!knownSnapshotPath) {
        writeJsonErrorResponse(socket, 404, QByteArrayLiteral("Not Found"), QStringLiteral("not_found"),
                               QStringLiteral("Unknown path."));
        return;
    }

    if (method != QByteArrayLiteral("GET")) {
        writeJsonErrorResponse(socket,
                               405,
                               QByteArrayLiteral("Method Not Allowed"),
                               QStringLiteral("method_not_allowed"),
                               QStringLiteral("Only GET is supported for this endpoint."),
                               {{QByteArrayLiteral("Allow"), QByteArrayLiteral("GET")}});
        return;
    }

    if (m_audioStateStore == nullptr || !m_audioStateStore->isReady()) {
        writeJsonErrorResponse(socket, 503, QByteArrayLiteral("Service Unavailable"), QStringLiteral("not_ready"),
                               QStringLiteral("Initial audio sink state is not ready yet."));
        return;
    }

    if (path == kSinksPath) {
        writeJsonResponse(socket, 200, QByteArrayLiteral("OK"),
                          QJsonDocument(plasma_bridge::toJsonObject(m_audioStateStore->audioState())));
        return;
    }

    const std::optional<plasma_bridge::AudioSinkState> defaultSink = m_audioStateStore->defaultSink();
    if (!defaultSink.has_value()) {
        writeJsonErrorResponse(socket, 404, QByteArrayLiteral("Not Found"), QStringLiteral("not_found"),
                               QStringLiteral("No default audio sink available."));
        return;
    }

    writeJsonResponse(socket, 200, QByteArrayLiteral("OK"), QJsonDocument(plasma_bridge::toJsonObject(*defaultSink)));
}

void SnapshotHttpServer::writeByteResponse(QTcpSocket *socket,
                                           int statusCode,
                                           const QByteArray &reasonPhrase,
                                           const QByteArray &contentType,
                                           const QByteArray &body,
                                           const QList<QPair<QByteArray, QByteArray>> &extraHeaders)
{
    if (socket == nullptr) {
        return;
    }

    socket->write(buildHttpResponse(statusCode, reasonPhrase, contentType, body, extraHeaders));
    socket->disconnectFromHost();
}

void SnapshotHttpServer::writeJsonResponse(QTcpSocket *socket,
                                           int statusCode,
                                           const QByteArray &reasonPhrase,
                                           const QJsonDocument &document,
                                           const QList<QPair<QByteArray, QByteArray>> &extraHeaders)
{
    const QByteArray body = document.toJson(QJsonDocument::Compact);
    writeByteResponse(socket,
                      statusCode,
                      reasonPhrase,
                      QByteArrayLiteral("application/json; charset=utf-8"),
                      body,
                      extraHeaders);
}

void SnapshotHttpServer::writeJsonErrorResponse(QTcpSocket *socket,
                                                int statusCode,
                                                const QByteArray &reasonPhrase,
                                                const QString &code,
                                                const QString &message,
                                                const QList<QPair<QByteArray, QByteArray>> &extraHeaders)
{
    QJsonObject body;
    body[QStringLiteral("ok")] = false;
    body[QStringLiteral("code")] = code;
    body[QStringLiteral("message")] = message;

    writeJsonResponse(socket, statusCode, reasonPhrase, QJsonDocument(body), extraHeaders);
}

} // namespace plasma_bridge::api
