#include "api/snapshot_http_server.h"

#include "common/audio_state.h"
#include "control/audio_volume_controller.h"
#include "state/audio_state_store.h"

#include <QFile>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTcpSocket>
#include <QUrl>

namespace plasma_bridge::api
{
namespace
{

constexpr qsizetype kMaxRequestBytes = 16 * 1024;
const QByteArray kHeaderDelimiter = QByteArrayLiteral("\r\n\r\n");
const QString kSinksPath = QStringLiteral("/snapshot/audio/sinks");
const QString kDefaultSinkPath = QStringLiteral("/snapshot/audio/default-sink");
const QString kSourcesPath = QStringLiteral("/snapshot/audio/sources");
const QString kDefaultSourcePath = QStringLiteral("/snapshot/audio/default-source");
const QString kControlSinksPrefix = QStringLiteral("/control/audio/sinks/");
const QString kVolumePathSuffix = QStringLiteral("/volume");
const QString kVolumeIncrementPathSuffix = QStringLiteral("/volume/increment");
const QString kVolumeDecrementPathSuffix = QStringLiteral("/volume/decrement");
const QString kDocsRootPath = QStringLiteral("/docs/");
const QString kDocsRootNoSlashPath = QStringLiteral("/docs");
const QString kDocsHttpPath = QStringLiteral("/docs/http");
const QString kDocsWsPath = QStringLiteral("/docs/ws");
const QString kDocsOpenApiPath = QStringLiteral("/docs/openapi.yaml");
const QString kDocsAsyncApiPath = QStringLiteral("/docs/asyncapi.yaml");
const QString kDocsNoticesPath = QStringLiteral("/docs/assets/THIRD_PARTY_NOTICES.txt");
const QString kDefaultOpenApiServerUrl = QStringLiteral("http://127.0.0.1:8080");
const QString kDefaultAsyncApiHost = QStringLiteral("127.0.0.1:8081");

enum class VolumeControlOperation {
    Set,
    Increment,
    Decrement,
};

enum class SnapshotRoute {
    None,
    Sinks,
    DefaultSink,
    Sources,
    DefaultSource,
};

enum class ControlTargetMatch {
    NoMatch,
    Match,
    InvalidSinkId,
};

struct ControlTarget {
    VolumeControlOperation operation = VolumeControlOperation::Set;
    QString sinkId;
};

struct ControlTargetParseResult {
    ControlTargetMatch match = ControlTargetMatch::NoMatch;
    ControlTarget target;
};

SnapshotRoute snapshotRouteForPath(const QString &path)
{
    if (path == kSinksPath) {
        return SnapshotRoute::Sinks;
    }
    if (path == kDefaultSinkPath) {
        return SnapshotRoute::DefaultSink;
    }
    if (path == kSourcesPath) {
        return SnapshotRoute::Sources;
    }
    if (path == kDefaultSourcePath) {
        return SnapshotRoute::DefaultSource;
    }

    return SnapshotRoute::None;
}

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

bool parseHeaders(const QList<QByteArray> &lines, QHash<QByteArray, QByteArray> *outHeaders, QString *errorMessage)
{
    if (outHeaders == nullptr) {
        return false;
    }

    outHeaders->clear();

    for (qsizetype i = 1; i < lines.size(); ++i) {
        const QByteArray line = lines.at(i).trimmed();
        if (line.isEmpty()) {
            continue;
        }

        const qsizetype separatorIndex = line.indexOf(':');
        if (separatorIndex <= 0) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Malformed HTTP header.");
            }
            return false;
        }

        const QByteArray name = line.left(separatorIndex).trimmed().toLower();
        const QByteArray value = line.mid(separatorIndex + 1).trimmed();
        if (name.isEmpty()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Malformed HTTP header.");
            }
            return false;
        }

        outHeaders->insert(name, value);
    }

    return true;
}

bool parseContentLength(const QHash<QByteArray, QByteArray> &headers,
                        qsizetype *outLength,
                        bool *outPresent,
                        QString *errorMessage)
{
    if (outLength == nullptr) {
        return false;
    }

    if (outPresent != nullptr) {
        *outPresent = false;
    }

    *outLength = 0;

    if (headers.contains(QByteArrayLiteral("transfer-encoding"))) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Transfer-Encoding is not supported.");
        }
        return false;
    }

    const auto it = headers.constFind(QByteArrayLiteral("content-length"));
    if (it == headers.cend()) {
        return true;
    }

    if (outPresent != nullptr) {
        *outPresent = true;
    }

    bool ok = false;
    const qlonglong contentLength = it.value().toLongLong(&ok);
    if (!ok || contentLength < 0) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Content-Length is invalid.");
        }
        return false;
    }

    if (contentLength > kMaxRequestBytes) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Request is too large.");
        }
        return false;
    }

    *outLength = static_cast<qsizetype>(contentLength);
    return true;
}

bool isJsonContentType(const QByteArray &contentType)
{
    const QByteArray mediaType = contentType.split(';').first().trimmed().toLower();
    return mediaType == QByteArrayLiteral("application/json");
}

ControlTargetParseResult parseControlTarget(const QString &path)
{
    ControlTargetParseResult result;
    if (!path.startsWith(kControlSinksPrefix)) {
        return result;
    }

    const auto trySuffix = [&](const QString &suffix, const VolumeControlOperation operation) -> bool {
        if (!path.endsWith(suffix)) {
            return false;
        }

        const qsizetype encodedSinkIdLength = path.size() - kControlSinksPrefix.size() - suffix.size();
        if (encodedSinkIdLength <= 0) {
            return false;
        }

        const QString encodedSinkId = path.mid(kControlSinksPrefix.size(), encodedSinkIdLength);
        if (encodedSinkId.contains(QLatin1Char('/'))) {
            return false;
        }

        const QString sinkId = QUrl::fromPercentEncoding(encodedSinkId.toUtf8());
        if (sinkId.isEmpty() || sinkId.contains(QLatin1Char('/'))) {
            result.match = ControlTargetMatch::InvalidSinkId;
            return true;
        }

        result.match = ControlTargetMatch::Match;
        result.target.operation = operation;
        result.target.sinkId = sinkId;
        return true;
    };

    if (trySuffix(kVolumeIncrementPathSuffix, VolumeControlOperation::Increment)) {
        return result;
    }
    if (trySuffix(kVolumeDecrementPathSuffix, VolumeControlOperation::Decrement)) {
        return result;
    }
    if (trySuffix(kVolumePathSuffix, VolumeControlOperation::Set)) {
        return result;
    }

    return result;
}

int httpStatusCodeForVolumeChangeStatus(const control::VolumeChangeStatus status)
{
    switch (status) {
    case control::VolumeChangeStatus::Accepted:
        return 200;
    case control::VolumeChangeStatus::InvalidValue:
        return 400;
    case control::VolumeChangeStatus::SinkNotFound:
        return 404;
    case control::VolumeChangeStatus::SinkNotWritable:
        return 409;
    case control::VolumeChangeStatus::NotReady:
        return 503;
    }

    return 400;
}

QByteArray reasonPhraseForStatusCode(const int statusCode)
{
    switch (statusCode) {
    case 200:
        return QByteArrayLiteral("OK");
    case 400:
        return QByteArrayLiteral("Bad Request");
    case 404:
        return QByteArrayLiteral("Not Found");
    case 405:
        return QByteArrayLiteral("Method Not Allowed");
    case 409:
        return QByteArrayLiteral("Conflict");
    case 415:
        return QByteArrayLiteral("Unsupported Media Type");
    case 503:
        return QByteArrayLiteral("Service Unavailable");
    default:
        return QByteArrayLiteral("Bad Request");
    }
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
                                       control::AudioVolumeController *audioVolumeController,
                                       const QString &documentationHost,
                                       quint16 documentationHttpPort,
                                       quint16 documentationWsPort,
                                       QObject *parent)
    : QObject(parent)
    , m_audioStateStore(audioStateStore)
    , m_audioVolumeController(audioVolumeController)
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

    const QByteArray headerData = buffer.left(headerEnd);
    const QList<QByteArray> headerLines = headerData.split('\n');
    QHash<QByteArray, QByteArray> headers;
    QString headerErrorMessage;
    if (!parseHeaders(headerLines, &headers, &headerErrorMessage)) {
        writeJsonErrorResponse(socket,
                               400,
                               QByteArrayLiteral("Bad Request"),
                               QStringLiteral("bad_request"),
                               headerErrorMessage);
        m_pendingRequests.remove(socket);
        return;
    }

    qsizetype contentLength = 0;
    if (!parseContentLength(headers, &contentLength, nullptr, &headerErrorMessage)) {
        writeJsonErrorResponse(socket,
                               400,
                               QByteArrayLiteral("Bad Request"),
                               QStringLiteral("bad_request"),
                               headerErrorMessage);
        m_pendingRequests.remove(socket);
        return;
    }

    const qsizetype totalRequestBytes = headerEnd + kHeaderDelimiter.size() + contentLength;
    if (totalRequestBytes > kMaxRequestBytes) {
        writeJsonErrorResponse(socket,
                               400,
                               QByteArrayLiteral("Bad Request"),
                               QStringLiteral("bad_request"),
                               QStringLiteral("Request is too large."));
        m_pendingRequests.remove(socket);
        return;
    }

    if (buffer.size() < totalRequestBytes) {
        return;
    }

    const QByteArray requestData = buffer.left(totalRequestBytes);
    m_pendingRequests.remove(socket);
    processRequest(socket, requestData);
}

void SnapshotHttpServer::processRequest(QTcpSocket *socket, const QByteArray &requestData)
{
    if (requestData.size() > kMaxRequestBytes) {
        writeJsonErrorResponse(socket,
                               400,
                               QByteArrayLiteral("Bad Request"),
                               QStringLiteral("bad_request"),
                               QStringLiteral("Request is too large."));
        return;
    }

    const qsizetype headerEnd = requestData.indexOf(kHeaderDelimiter);
    const QByteArray headerData = headerEnd >= 0 ? requestData.left(headerEnd) : requestData;
    const QByteArray bodyData = headerEnd >= 0 ? requestData.mid(headerEnd + kHeaderDelimiter.size()) : QByteArray();

    const QList<QByteArray> lines = headerData.split('\n');
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
    QHash<QByteArray, QByteArray> headers;
    QString headerErrorMessage;
    if (!parseHeaders(lines, &headers, &headerErrorMessage)) {
        writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                               headerErrorMessage);
        return;
    }

    bool hasContentLength = false;
    qsizetype contentLength = 0;
    if (!parseContentLength(headers, &contentLength, &hasContentLength, &headerErrorMessage)) {
        writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                               headerErrorMessage);
        return;
    }

    if (bodyData.size() < contentLength) {
        writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                               QStringLiteral("Incomplete HTTP request body."));
        return;
    }

    const QByteArray body = contentLength > 0 ? bodyData.left(contentLength) : QByteArray();

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

    const ControlTargetParseResult controlTarget = parseControlTarget(path);
    if (controlTarget.match == ControlTargetMatch::InvalidSinkId) {
        writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                               QStringLiteral("Sink id must be a single path segment."));
        return;
    }

    if (controlTarget.match == ControlTargetMatch::Match) {
        if (method != QByteArrayLiteral("POST")) {
            writeJsonErrorResponse(socket,
                                   405,
                                   QByteArrayLiteral("Method Not Allowed"),
                                   QStringLiteral("method_not_allowed"),
                                   QStringLiteral("Only POST is supported for this endpoint."),
                                   {{QByteArrayLiteral("Allow"), QByteArrayLiteral("POST")}});
            return;
        }

        if (m_audioVolumeController == nullptr) {
            writeJsonErrorResponse(socket, 503, QByteArrayLiteral("Service Unavailable"), QStringLiteral("not_ready"),
                                   QStringLiteral("Audio volume control is not available."));
            return;
        }

        if (!hasContentLength) {
            writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                                   QStringLiteral("Content-Length is required for POST requests."));
            return;
        }

        if (!isJsonContentType(headers.value(QByteArrayLiteral("content-type")))) {
            writeJsonErrorResponse(socket,
                                   415,
                                   QByteArrayLiteral("Unsupported Media Type"),
                                   QStringLiteral("unsupported_media_type"),
                                   QStringLiteral("Content-Type must be application/json."));
            return;
        }

        if (body.isEmpty()) {
            writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                                   QStringLiteral("Request body is required."));
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                                   QStringLiteral("Request body must be valid JSON."));
            return;
        }

        if (!document.isObject()) {
            writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                                   QStringLiteral("Request body must be a JSON object."));
            return;
        }

        const QJsonObject object = document.object();
        const QJsonValue valueField = object.value(QStringLiteral("value"));
        if (!valueField.isDouble()) {
            writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                                   QStringLiteral("Request body must contain a numeric value field."));
            return;
        }

        control::VolumeChangeResult result;
        switch (controlTarget.target.operation) {
        case VolumeControlOperation::Set:
            result = m_audioVolumeController->setVolume(controlTarget.target.sinkId, valueField.toDouble());
            break;
        case VolumeControlOperation::Increment:
            result = m_audioVolumeController->incrementVolume(controlTarget.target.sinkId, valueField.toDouble());
            break;
        case VolumeControlOperation::Decrement:
            result = m_audioVolumeController->decrementVolume(controlTarget.target.sinkId, valueField.toDouble());
            break;
        }

        const int statusCode = httpStatusCodeForVolumeChangeStatus(result.status);
        writeJsonResponse(socket,
                          statusCode,
                          reasonPhraseForStatusCode(statusCode),
                          QJsonDocument(control::toJsonObject(result)));
        return;
    }

    const SnapshotRoute snapshotRoute = snapshotRouteForPath(path);
    if (snapshotRoute == SnapshotRoute::None) {
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
                               QStringLiteral("Initial audio state is not ready yet."));
        return;
    }

    switch (snapshotRoute) {
    case SnapshotRoute::Sinks:
        writeJsonResponse(socket, 200, QByteArrayLiteral("OK"),
                          QJsonDocument(plasma_bridge::toJsonObject(plasma_bridge::toAudioOutputState(m_audioStateStore->audioState()))));
        return;
    case SnapshotRoute::Sources:
        writeJsonResponse(socket, 200, QByteArrayLiteral("OK"),
                          QJsonDocument(plasma_bridge::toJsonObject(plasma_bridge::toAudioInputState(m_audioStateStore->audioState()))));
        return;
    case SnapshotRoute::DefaultSink: {
        const std::optional<plasma_bridge::AudioSinkState> defaultSink = m_audioStateStore->defaultSink();
        if (!defaultSink.has_value()) {
            writeJsonErrorResponse(socket, 404, QByteArrayLiteral("Not Found"), QStringLiteral("not_found"),
                                   QStringLiteral("No default audio sink available."));
            return;
        }

        writeJsonResponse(socket, 200, QByteArrayLiteral("OK"), QJsonDocument(plasma_bridge::toJsonObject(*defaultSink)));
        return;
    }
    case SnapshotRoute::DefaultSource: {
        const std::optional<plasma_bridge::AudioSourceState> defaultSource = m_audioStateStore->defaultSource();
        if (!defaultSource.has_value()) {
            writeJsonErrorResponse(socket, 404, QByteArrayLiteral("Not Found"), QStringLiteral("not_found"),
                                   QStringLiteral("No default audio source available."));
            return;
        }

        writeJsonResponse(socket, 200, QByteArrayLiteral("OK"), QJsonDocument(plasma_bridge::toJsonObject(*defaultSource)));
        return;
    }
    case SnapshotRoute::None:
        break;
    }
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
