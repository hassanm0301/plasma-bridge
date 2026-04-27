#include "api/snapshot_http_server.h"

#include "api/audio_control_http_helpers.h"
#include "api/json_envelope.h"
#include "api/window_control_http_helpers.h"
#include "common/audio_state.h"
#include "common/window_state.h"
#include "control/audio_device_controller.h"
#include "control/audio_volume_controller.h"
#include "control/window_activation_controller.h"
#include "plasma_bridge_build_config.h"
#include "state/audio_state_store.h"
#include "state/window_state_store.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSettings>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QUrl>

#include <algorithm>
#include <cstdlib>

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
const QString kWindowsPath = QStringLiteral("/snapshot/windows");
const QString kActiveWindowPath = QStringLiteral("/snapshot/windows/active");
const QString kAppIconPathPrefix = QStringLiteral("/icons/apps/");
const QString kDocsRootPath = QStringLiteral("/docs/");
const QString kDocsRootNoSlashPath = QStringLiteral("/docs");
const QString kDocsHttpPath = QStringLiteral("/docs/http");
const QString kDocsWsPath = QStringLiteral("/docs/ws");
const QString kDocsOpenApiPath = QStringLiteral("/docs/openapi.yaml");
const QString kDocsAsyncApiPath = QStringLiteral("/docs/asyncapi.yaml");
const QString kDocsNoticesPath = QStringLiteral("/docs/assets/THIRD_PARTY_NOTICES.txt");
const QString kDefaultOpenApiServerUrl =
    QStringLiteral("http://%1:%2").arg(QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST),
                                        QString::number(PLASMA_BRIDGE_DEFAULT_HTTP_PORT));
const QString kDefaultAsyncApiHost =
    QStringLiteral("%1:%2").arg(QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST),
                                 QString::number(PLASMA_BRIDGE_DEFAULT_WS_PORT));

enum class SnapshotRoute {
    None,
    Sinks,
    DefaultSink,
    Sources,
    DefaultSource,
    Windows,
    ActiveWindow,
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
    if (path == kWindowsPath) {
        return SnapshotRoute::Windows;
    }
    if (path == kActiveWindowPath) {
        return SnapshotRoute::ActiveWindow;
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

QByteArray contentTypeForIconPath(const QString &path)
{
    if (path.endsWith(QStringLiteral(".svg")) || path.endsWith(QStringLiteral(".svgz"))) {
        return QByteArrayLiteral("image/svg+xml");
    }
    if (path.endsWith(QStringLiteral(".png"))) {
        return QByteArrayLiteral("image/png");
    }
    if (path.endsWith(QStringLiteral(".jpg")) || path.endsWith(QStringLiteral(".jpeg"))) {
        return QByteArrayLiteral("image/jpeg");
    }
    if (path.endsWith(QStringLiteral(".xpm"))) {
        return QByteArrayLiteral("image/x-xpixmap");
    }
    return QByteArrayLiteral("application/octet-stream");
}

QByteArray readResourceBytes(const QString &resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    return file.readAll();
}

QString configuredIconThemeName()
{
    const QString configPath = QStandardPaths::locate(QStandardPaths::ConfigLocation, QStringLiteral("kdeglobals"));
    if (configPath.isEmpty()) {
        return QStringLiteral("breeze");
    }

    QSettings settings(configPath, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Icons"));
    const QString themeName = settings.value(QStringLiteral("Theme"), QStringLiteral("breeze")).toString();
    settings.endGroup();
    return themeName.isEmpty() ? QStringLiteral("breeze") : themeName;
}

QStringList iconSearchRoots()
{
    QStringList roots = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    roots.removeDuplicates();
    return roots;
}

int iconCandidateScore(const QFileInfo &fileInfo)
{
    const QString suffix = fileInfo.suffix().toLower();
    int score = suffix == QStringLiteral("svg") || suffix == QStringLiteral("svgz") ? 1000 : 0;

    QDir dir = fileInfo.dir();
    while (!dir.isRoot()) {
        bool ok = false;
        const int size = dir.dirName().toInt(&ok);
        if (ok) {
            score += 256 - std::min(std::abs(size - 48), 255);
            break;
        }
        dir.cdUp();
    }

    return score;
}

QString bestIconCandidate(const QStringList &candidatePaths)
{
    QString bestPath;
    int bestScore = -1;
    for (const QString &path : candidatePaths) {
        const QFileInfo fileInfo(path);
        const int score = iconCandidateScore(fileInfo);
        if (score > bestScore) {
            bestScore = score;
            bestPath = fileInfo.absoluteFilePath();
        }
    }
    return bestPath;
}

QString findIconInDirectory(const QString &directoryPath, const QString &iconName)
{
    const QDir directory(directoryPath);
    if (!directory.exists()) {
        return {};
    }

    const QStringList nameFilters{
        iconName + QStringLiteral(".svg"),
        iconName + QStringLiteral(".svgz"),
        iconName + QStringLiteral(".png"),
        iconName + QStringLiteral(".jpg"),
        iconName + QStringLiteral(".jpeg"),
        iconName + QStringLiteral(".xpm"),
    };

    QStringList candidates;
    QDirIterator iterator(directory.absolutePath(), nameFilters, QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        candidates.append(iterator.next());
    }

    return bestIconCandidate(candidates);
}

QString resolveAppIconPath(const QString &iconName)
{
    const QString decodedIconName = iconName.trimmed();
    if (decodedIconName.isEmpty() || decodedIconName.contains(QLatin1Char('/'))) {
        return {};
    }

    const QString configuredTheme = configuredIconThemeName();
    const QStringList themeNames{configuredTheme, QStringLiteral("breeze"), QStringLiteral("hicolor")};
    for (const QString &root : iconSearchRoots()) {
        for (const QString &themeName : themeNames) {
            const QString iconPath = findIconInDirectory(QDir(root).filePath(QStringLiteral("icons/%1").arg(themeName)),
                                                         decodedIconName);
            if (!iconPath.isEmpty()) {
                return iconPath;
            }
        }

        const QString pixmapPath = findIconInDirectory(QDir(root).filePath(QStringLiteral("pixmaps")), decodedIconName);
        if (!pixmapPath.isEmpty()) {
            return pixmapPath;
        }
    }

    return {};
}

QString appIconNameFromPath(const QString &path)
{
    if (!path.startsWith(kAppIconPathPrefix)) {
        return {};
    }

    const QString encodedIconName = path.mid(kAppIconPathPrefix.size());
    if (encodedIconName.isEmpty() || encodedIconName.contains(QLatin1Char('/'))) {
        return {};
    }

    const QString iconName = QUrl::fromPercentEncoding(encodedIconName.toUtf8());
    if (iconName.isEmpty() || iconName.contains(QLatin1Char('/'))) {
        return {};
    }

    return iconName;
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

bool isAllowedCorsOrigin(const QByteArray &origin)
{
    const QUrl originUrl = QUrl::fromEncoded(origin.trimmed());
    const QString path = originUrl.path();
    if (!originUrl.isValid() || (!path.isEmpty() && path != QStringLiteral("/")) || originUrl.hasQuery()
        || originUrl.hasFragment()) {
        return false;
    }

    const QString scheme = originUrl.scheme();
    if (scheme != QStringLiteral("http") && scheme != QStringLiteral("https")) {
        return false;
    }

    const QString host = originUrl.host();
    return host == QStringLiteral("127.0.0.1") || host == QStringLiteral("localhost") || host == QStringLiteral("::1");
}

QList<QPair<QByteArray, QByteArray>> corsHeadersForRequest(const QHash<QByteArray, QByteArray> &headers)
{
    const QByteArray origin = headers.value(QByteArrayLiteral("origin")).trimmed();
    if (!isAllowedCorsOrigin(origin)) {
        return {};
    }

    return {
        {QByteArrayLiteral("Access-Control-Allow-Origin"), origin},
        {QByteArrayLiteral("Vary"), QByteArrayLiteral("Origin")},
    };
}

QList<QPair<QByteArray, QByteArray>> corsPreflightHeadersForRequest(const QHash<QByteArray, QByteArray> &headers)
{
    if (!isAllowedCorsOrigin(headers.value(QByteArrayLiteral("origin")))) {
        return {};
    }

    QList<QPair<QByteArray, QByteArray>> corsHeaders;
    corsHeaders.append({QByteArrayLiteral("Access-Control-Allow-Methods"), QByteArrayLiteral("GET, POST, OPTIONS")});
    corsHeaders.append({QByteArrayLiteral("Access-Control-Allow-Headers"), QByteArrayLiteral("Content-Type")});
    corsHeaders.append({QByteArrayLiteral("Access-Control-Max-Age"), QByteArrayLiteral("600")});
    return corsHeaders;
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

QString errorCodeForVolumeChangeStatus(const control::VolumeChangeStatus status)
{
    switch (status) {
    case control::VolumeChangeStatus::Accepted:
        return QStringLiteral("accepted");
    case control::VolumeChangeStatus::NotReady:
        return QStringLiteral("not_ready");
    case control::VolumeChangeStatus::SinkNotFound:
        return QStringLiteral("sink_not_found");
    case control::VolumeChangeStatus::SinkNotWritable:
        return QStringLiteral("sink_not_writable");
    case control::VolumeChangeStatus::InvalidValue:
        return QStringLiteral("invalid_value");
    }

    return QStringLiteral("invalid_value");
}

QString errorMessageForVolumeChangeStatus(const control::VolumeChangeStatus status)
{
    switch (status) {
    case control::VolumeChangeStatus::Accepted:
        return QStringLiteral("Volume change request accepted.");
    case control::VolumeChangeStatus::NotReady:
        return QStringLiteral("Volume control is not ready yet.");
    case control::VolumeChangeStatus::SinkNotFound:
        return QStringLiteral("Requested sink was not found.");
    case control::VolumeChangeStatus::SinkNotWritable:
        return QStringLiteral("Sink exists but cannot be written to.");
    case control::VolumeChangeStatus::InvalidValue:
        return QStringLiteral("Requested volume value is invalid.");
    }

    return QStringLiteral("Requested volume value is invalid.");
}

QString errorCodeForAudioDeviceChangeStatus(const control::AudioDeviceChangeStatus status)
{
    switch (status) {
    case control::AudioDeviceChangeStatus::Accepted:
        return QStringLiteral("accepted");
    case control::AudioDeviceChangeStatus::NotReady:
        return QStringLiteral("not_ready");
    case control::AudioDeviceChangeStatus::SinkNotFound:
        return QStringLiteral("sink_not_found");
    case control::AudioDeviceChangeStatus::SourceNotFound:
        return QStringLiteral("source_not_found");
    case control::AudioDeviceChangeStatus::SinkNotWritable:
        return QStringLiteral("sink_not_writable");
    case control::AudioDeviceChangeStatus::SourceNotWritable:
        return QStringLiteral("source_not_writable");
    }

    return QStringLiteral("not_ready");
}

QString errorMessageForDefaultDeviceChangeStatus(const AudioControlTargetKind targetKind,
                                                 const control::AudioDeviceChangeStatus status)
{
    switch (status) {
    case control::AudioDeviceChangeStatus::Accepted:
        return QStringLiteral("Default device change request accepted.");
    case control::AudioDeviceChangeStatus::NotReady:
        return QStringLiteral("Audio device control is not ready yet.");
    case control::AudioDeviceChangeStatus::SinkNotFound:
        return QStringLiteral("Requested sink was not found.");
    case control::AudioDeviceChangeStatus::SourceNotFound:
        return QStringLiteral("Requested source was not found.");
    case control::AudioDeviceChangeStatus::SinkNotWritable:
        return targetKind == AudioControlTargetKind::Sink
            ? QStringLiteral("Sink exists but cannot be selected as default.")
            : QStringLiteral("Requested sink was not found.");
    case control::AudioDeviceChangeStatus::SourceNotWritable:
        return targetKind == AudioControlTargetKind::Source
            ? QStringLiteral("Source exists but cannot be selected as default.")
            : QStringLiteral("Requested source was not found.");
    }

    return QStringLiteral("Audio device control is not ready yet.");
}

QString errorMessageForMuteChangeStatus(const AudioControlTargetKind targetKind,
                                        const control::AudioDeviceChangeStatus status)
{
    switch (status) {
    case control::AudioDeviceChangeStatus::Accepted:
        return QStringLiteral("Mute change request accepted.");
    case control::AudioDeviceChangeStatus::NotReady:
        return QStringLiteral("Audio device control is not ready yet.");
    case control::AudioDeviceChangeStatus::SinkNotFound:
        return QStringLiteral("Requested sink was not found.");
    case control::AudioDeviceChangeStatus::SourceNotFound:
        return QStringLiteral("Requested source was not found.");
    case control::AudioDeviceChangeStatus::SinkNotWritable:
        return targetKind == AudioControlTargetKind::Sink
            ? QStringLiteral("Sink exists but cannot be muted.")
            : QStringLiteral("Requested sink was not found.");
    case control::AudioDeviceChangeStatus::SourceNotWritable:
        return targetKind == AudioControlTargetKind::Source
            ? QStringLiteral("Source exists but cannot be muted.")
            : QStringLiteral("Requested source was not found.");
    }

    return QStringLiteral("Audio device control is not ready yet.");
}

QString errorCodeForWindowActivationStatus(const control::WindowActivationStatus status)
{
    switch (status) {
    case control::WindowActivationStatus::Accepted:
        return QStringLiteral("accepted");
    case control::WindowActivationStatus::NotReady:
        return QStringLiteral("not_ready");
    case control::WindowActivationStatus::WindowNotFound:
        return QStringLiteral("window_not_found");
    case control::WindowActivationStatus::WindowNotActivatable:
        return QStringLiteral("window_not_activatable");
    }

    return QStringLiteral("not_ready");
}

QString errorMessageForWindowActivationStatus(const control::WindowActivationStatus status)
{
    switch (status) {
    case control::WindowActivationStatus::Accepted:
        return QStringLiteral("Window activation request accepted.");
    case control::WindowActivationStatus::NotReady:
        return QStringLiteral("Window activation control is not ready yet.");
    case control::WindowActivationStatus::WindowNotFound:
        return QStringLiteral("Requested window was not found.");
    case control::WindowActivationStatus::WindowNotActivatable:
        return QStringLiteral("Window exists but cannot be activated.");
    }

    return QStringLiteral("Window activation control is not ready yet.");
}

bool snapshotContainsWindow(const plasma_bridge::WindowSnapshot &snapshot, const QString &windowId)
{
    return std::any_of(snapshot.windows.cbegin(), snapshot.windows.cend(), [&windowId](const plasma_bridge::WindowState &window) {
        return window.id == windowId;
    });
}

} // namespace

SnapshotHttpServer::SnapshotHttpServer(state::AudioStateStore *audioStateStore,
                                       control::AudioVolumeController *audioVolumeController,
                                       control::AudioDeviceController *audioDeviceController,
                                       const QString &documentationHost,
                                       quint16 documentationHttpPort,
                                       quint16 documentationWsPort,
                                       QObject *parent)
    : SnapshotHttpServer(audioStateStore,
                         nullptr,
                         audioVolumeController,
                         audioDeviceController,
                         nullptr,
                         documentationHost,
                         documentationHttpPort,
                         documentationWsPort,
                         parent)
{
}

SnapshotHttpServer::SnapshotHttpServer(state::AudioStateStore *audioStateStore,
                                       state::WindowStateStore *windowStateStore,
                                       control::AudioVolumeController *audioVolumeController,
                                       control::AudioDeviceController *audioDeviceController,
                                       const QString &documentationHost,
                                       quint16 documentationHttpPort,
                                       quint16 documentationWsPort,
                                       QObject *parent)
    : SnapshotHttpServer(audioStateStore,
                         windowStateStore,
                         audioVolumeController,
                         audioDeviceController,
                         nullptr,
                         documentationHost,
                         documentationHttpPort,
                         documentationWsPort,
                         parent)
{
}

SnapshotHttpServer::SnapshotHttpServer(state::AudioStateStore *audioStateStore,
                                       state::WindowStateStore *windowStateStore,
                                       control::AudioVolumeController *audioVolumeController,
                                       control::AudioDeviceController *audioDeviceController,
                                       control::WindowActivationController *windowActivationController,
                                       const QString &documentationHost,
                                       quint16 documentationHttpPort,
                                       quint16 documentationWsPort,
                                       QObject *parent)
    : QObject(parent)
    , m_audioStateStore(audioStateStore)
    , m_windowStateStore(windowStateStore)
    , m_audioVolumeController(audioVolumeController)
    , m_audioDeviceController(audioDeviceController)
    , m_windowActivationController(windowActivationController)
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
            m_corsHeaders.remove(socket);
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

    m_corsHeaders.insert(socket, corsHeadersForRequest(headers));

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

    if (method == QByteArrayLiteral("OPTIONS")) {
        writeByteResponse(socket,
                          204,
                          QByteArrayLiteral("No Content"),
                          QByteArrayLiteral("text/plain; charset=utf-8"),
                          {},
                          corsPreflightHeadersForRequest(headers));
        return;
    }

    if (path.startsWith(kAppIconPathPrefix)) {
        if (method != QByteArrayLiteral("GET")) {
            writeJsonErrorResponse(socket,
                                   405,
                                   QByteArrayLiteral("Method Not Allowed"),
                                   QStringLiteral("method_not_allowed"),
                                   QStringLiteral("Only GET is supported for this endpoint."),
                                   {{QByteArrayLiteral("Allow"), QByteArrayLiteral("GET")}});
            return;
        }

        const QString iconName = appIconNameFromPath(path);
        if (iconName.isEmpty()) {
            writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                                   QStringLiteral("Icon name must be a single path segment."));
            return;
        }

        const QString iconPath = resolveAppIconPath(iconName);
        if (iconPath.isEmpty()) {
            writeJsonErrorResponse(socket, 404, QByteArrayLiteral("Not Found"), QStringLiteral("not_found"),
                                   QStringLiteral("Requested icon was not found."));
            return;
        }

        const QByteArray body = readResourceBytes(iconPath);
        if (body.isEmpty()) {
            writeJsonErrorResponse(socket, 404, QByteArrayLiteral("Not Found"), QStringLiteral("not_found"),
                                   QStringLiteral("Requested icon was not found."));
            return;
        }

        writeByteResponse(socket,
                          200,
                          QByteArrayLiteral("OK"),
                          contentTypeForIconPath(iconPath),
                          body,
                          {{QByteArrayLiteral("Cache-Control"), QByteArrayLiteral("public, max-age=86400")}});
        return;
    }

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

    const auto requireJsonObjectBody = [&](QJsonObject *outObject) -> bool {
        if (!hasContentLength) {
            writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                                   QStringLiteral("Content-Length is required for POST requests."));
            return false;
        }

        if (!isJsonContentType(headers.value(QByteArrayLiteral("content-type")))) {
            writeJsonErrorResponse(socket,
                                   415,
                                   QByteArrayLiteral("Unsupported Media Type"),
                                   QStringLiteral("unsupported_media_type"),
                                   QStringLiteral("Content-Type must be application/json."));
            return false;
        }

        if (body.isEmpty()) {
            writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                                   QStringLiteral("Request body is required."));
            return false;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                                   QStringLiteral("Request body must be valid JSON."));
            return false;
        }

        if (!document.isObject()) {
            writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                                   QStringLiteral("Request body must be a JSON object."));
            return false;
        }

        if (outObject != nullptr) {
            *outObject = document.object();
        }
        return true;
    };

    const AudioControlRouteParseResult controlRoute = parseAudioControlRoute(path);
    if (controlRoute.match == AudioControlRouteMatch::InvalidSinkId) {
        writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                               QStringLiteral("Sink id must be a single path segment."));
        return;
    }
    if (controlRoute.match == AudioControlRouteMatch::InvalidSourceId) {
        writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                               QStringLiteral("Source id must be a single path segment."));
        return;
    }

    if (controlRoute.match == AudioControlRouteMatch::Match) {
        if (method != QByteArrayLiteral("POST")) {
            writeJsonErrorResponse(socket,
                                   405,
                                   QByteArrayLiteral("Method Not Allowed"),
                                   QStringLiteral("method_not_allowed"),
                                   QStringLiteral("Only POST is supported for this endpoint."),
                                   {{QByteArrayLiteral("Allow"), QByteArrayLiteral("POST")}});
            return;
        }

        switch (controlRoute.route.operation) {
        case AudioControlOperation::SetVolume:
        case AudioControlOperation::IncrementVolume:
        case AudioControlOperation::DecrementVolume: {
            if (m_audioVolumeController == nullptr) {
                writeJsonErrorResponse(socket,
                                       503,
                                       QByteArrayLiteral("Service Unavailable"),
                                       QStringLiteral("not_ready"),
                                       QStringLiteral("Audio volume control is not available."));
                return;
            }

            QJsonObject object;
            if (!requireJsonObjectBody(&object)) {
                return;
            }

            const QJsonValue valueField = object.value(QStringLiteral("value"));
            if (!valueField.isDouble()) {
                writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                                       QStringLiteral("Request body must contain a numeric value field."));
                return;
            }

            control::VolumeChangeResult result;
            switch (controlRoute.route.operation) {
            case AudioControlOperation::SetVolume:
                result = m_audioVolumeController->setVolume(controlRoute.route.deviceId, valueField.toDouble());
                break;
            case AudioControlOperation::IncrementVolume:
                result = m_audioVolumeController->incrementVolume(controlRoute.route.deviceId, valueField.toDouble());
                break;
            case AudioControlOperation::DecrementVolume:
                result = m_audioVolumeController->decrementVolume(controlRoute.route.deviceId, valueField.toDouble());
                break;
            case AudioControlOperation::SetDefault:
            case AudioControlOperation::SetMute:
                break;
            }

            const int statusCode = httpStatusCodeForVolumeChangeStatus(result.status);
            if (result.status == control::VolumeChangeStatus::Accepted) {
                writeJsonResponse(socket,
                                  statusCode,
                                  reasonPhraseForStatusCode(statusCode),
                                  buildHttpSuccessEnvelope(buildVolumePayload(result)));
                return;
            }

            writeJsonErrorResponse(socket,
                                   statusCode,
                                   reasonPhraseForStatusCode(statusCode),
                                   errorCodeForVolumeChangeStatus(result.status),
                                   errorMessageForVolumeChangeStatus(result.status),
                                   {},
                                   buildVolumeErrorDetails(result));
            return;
        }
        case AudioControlOperation::SetDefault: {
            if (m_audioDeviceController == nullptr) {
                writeJsonErrorResponse(socket,
                                       503,
                                       QByteArrayLiteral("Service Unavailable"),
                                       QStringLiteral("not_ready"),
                                       QStringLiteral("Audio device control is not available."));
                return;
            }

            if (contentLength > 0) {
                writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                                       QStringLiteral("Request body is not supported for this endpoint."));
                return;
            }

            control::DefaultDeviceChangeResult result;
            switch (controlRoute.route.targetKind) {
            case AudioControlTargetKind::Sink:
                result = m_audioDeviceController->setDefaultSink(controlRoute.route.deviceId);
                break;
            case AudioControlTargetKind::Source:
                result = m_audioDeviceController->setDefaultSource(controlRoute.route.deviceId);
                break;
            }

            const int statusCode = httpStatusCodeForAudioDeviceChangeStatus(result.status);
            if (result.status == control::AudioDeviceChangeStatus::Accepted) {
                writeJsonResponse(socket,
                                  statusCode,
                                  reasonPhraseForStatusCode(statusCode),
                                  buildHttpSuccessEnvelope(buildDefaultDevicePayload(result)));
                return;
            }

            writeJsonErrorResponse(socket,
                                   statusCode,
                                   reasonPhraseForStatusCode(statusCode),
                                   errorCodeForAudioDeviceChangeStatus(result.status),
                                   errorMessageForDefaultDeviceChangeStatus(controlRoute.route.targetKind, result.status),
                                   {},
                                   buildDefaultDeviceErrorDetails(result));
            return;
        }
        case AudioControlOperation::SetMute: {
            if (m_audioDeviceController == nullptr) {
                writeJsonErrorResponse(socket,
                                       503,
                                       QByteArrayLiteral("Service Unavailable"),
                                       QStringLiteral("not_ready"),
                                       QStringLiteral("Audio device control is not available."));
                return;
            }

            QJsonObject object;
            if (!requireJsonObjectBody(&object)) {
                return;
            }

            const QJsonValue mutedField = object.value(QStringLiteral("muted"));
            if (!mutedField.isBool()) {
                writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                                       QStringLiteral("Request body must contain a boolean muted field."));
                return;
            }

            control::MuteChangeResult result;
            switch (controlRoute.route.targetKind) {
            case AudioControlTargetKind::Sink:
                result = m_audioDeviceController->setSinkMuted(controlRoute.route.deviceId, mutedField.toBool());
                break;
            case AudioControlTargetKind::Source:
                result = m_audioDeviceController->setSourceMuted(controlRoute.route.deviceId, mutedField.toBool());
                break;
            }

            const int statusCode = httpStatusCodeForAudioDeviceChangeStatus(result.status);
            if (result.status == control::AudioDeviceChangeStatus::Accepted) {
                writeJsonResponse(socket,
                                  statusCode,
                                  reasonPhraseForStatusCode(statusCode),
                                  buildHttpSuccessEnvelope(buildMutePayload(result)));
                return;
            }

            writeJsonErrorResponse(socket,
                                   statusCode,
                                   reasonPhraseForStatusCode(statusCode),
                                   errorCodeForAudioDeviceChangeStatus(result.status),
                                   errorMessageForMuteChangeStatus(controlRoute.route.targetKind, result.status),
                                   {},
                                   buildMuteErrorDetails(result));
            return;
        }
        }
    }

    const WindowControlRouteParseResult windowControlRoute = parseWindowControlRoute(path);
    if (windowControlRoute.match == WindowControlRouteMatch::InvalidWindowId) {
        writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                               QStringLiteral("Window id must be a single path segment."));
        return;
    }

    if (windowControlRoute.match == WindowControlRouteMatch::Match) {
        if (method != QByteArrayLiteral("POST")) {
            writeJsonErrorResponse(socket,
                                   405,
                                   QByteArrayLiteral("Method Not Allowed"),
                                   QStringLiteral("method_not_allowed"),
                                   QStringLiteral("Only POST is supported for this endpoint."),
                                   {{QByteArrayLiteral("Allow"), QByteArrayLiteral("POST")}});
            return;
        }

        if (contentLength > 0) {
            writeJsonErrorResponse(socket, 400, QByteArrayLiteral("Bad Request"), QStringLiteral("bad_request"),
                                   QStringLiteral("Request body is not supported for this endpoint."));
            return;
        }

        if (m_windowStateStore == nullptr || !m_windowStateStore->isReady()) {
            writeJsonErrorResponse(socket, 503, QByteArrayLiteral("Service Unavailable"), QStringLiteral("not_ready"),
                                   QStringLiteral("Initial window state is not ready yet."));
            return;
        }

        if (m_windowActivationController == nullptr) {
            writeJsonErrorResponse(socket,
                                   503,
                                   QByteArrayLiteral("Service Unavailable"),
                                   QStringLiteral("not_ready"),
                                   QStringLiteral("Window activation control is not available."));
            return;
        }

        if (!snapshotContainsWindow(m_windowStateStore->windowState(), windowControlRoute.route.windowId)) {
            control::WindowActivationResult result;
            result.status = control::WindowActivationStatus::WindowNotFound;
            result.windowId = windowControlRoute.route.windowId;
            const int statusCode = httpStatusCodeForWindowActivationStatus(result.status);
            writeJsonErrorResponse(socket,
                                   statusCode,
                                   reasonPhraseForStatusCode(statusCode),
                                   errorCodeForWindowActivationStatus(result.status),
                                   errorMessageForWindowActivationStatus(result.status),
                                   {},
                                   buildWindowActivationErrorDetails(result));
            return;
        }

        control::WindowActivationResult result =
            m_windowActivationController->activateWindow(windowControlRoute.route.windowId);
        if (result.windowId.isEmpty()) {
            result.windowId = windowControlRoute.route.windowId;
        }

        const int statusCode = httpStatusCodeForWindowActivationStatus(result.status);
        if (result.status == control::WindowActivationStatus::Accepted) {
            writeJsonResponse(socket,
                              statusCode,
                              reasonPhraseForStatusCode(statusCode),
                              buildHttpSuccessEnvelope(buildWindowActivationPayload(result)));
            return;
        }

        writeJsonErrorResponse(socket,
                               statusCode,
                               reasonPhraseForStatusCode(statusCode),
                               errorCodeForWindowActivationStatus(result.status),
                               errorMessageForWindowActivationStatus(result.status),
                               {},
                               buildWindowActivationErrorDetails(result));
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

    switch (snapshotRoute) {
    case SnapshotRoute::Windows:
        if (m_windowStateStore == nullptr || !m_windowStateStore->isReady()) {
            writeJsonErrorResponse(socket, 503, QByteArrayLiteral("Service Unavailable"), QStringLiteral("not_ready"),
                                   QStringLiteral("Initial window state is not ready yet."));
            return;
        }

        writeJsonResponse(socket,
                          200,
                          QByteArrayLiteral("OK"),
                          buildHttpSuccessEnvelope(plasma_bridge::toJsonObject(m_windowStateStore->windowState())));
        return;
    case SnapshotRoute::ActiveWindow: {
        if (m_windowStateStore == nullptr || !m_windowStateStore->isReady()) {
            writeJsonErrorResponse(socket, 503, QByteArrayLiteral("Service Unavailable"), QStringLiteral("not_ready"),
                                   QStringLiteral("Initial window state is not ready yet."));
            return;
        }

        const std::optional<plasma_bridge::WindowState> active = m_windowStateStore->activeWindow();
        QJsonObject payload;
        payload[QStringLiteral("window")] =
            active.has_value() ? QJsonValue(plasma_bridge::toJsonObject(*active)) : QJsonValue(QJsonValue::Null);
        writeJsonResponse(socket, 200, QByteArrayLiteral("OK"), buildHttpSuccessEnvelope(payload));
        return;
    }
    case SnapshotRoute::Sinks:
        if (m_audioStateStore == nullptr || !m_audioStateStore->isReady()) {
            writeJsonErrorResponse(socket, 503, QByteArrayLiteral("Service Unavailable"), QStringLiteral("not_ready"),
                                   QStringLiteral("Initial audio state is not ready yet."));
            return;
        }

        writeJsonResponse(socket,
                          200,
                          QByteArrayLiteral("OK"),
                          buildHttpSuccessEnvelope(
                              plasma_bridge::toJsonObject(plasma_bridge::toAudioOutputState(m_audioStateStore->audioState()))));
        return;
    case SnapshotRoute::Sources:
        if (m_audioStateStore == nullptr || !m_audioStateStore->isReady()) {
            writeJsonErrorResponse(socket, 503, QByteArrayLiteral("Service Unavailable"), QStringLiteral("not_ready"),
                                   QStringLiteral("Initial audio state is not ready yet."));
            return;
        }

        writeJsonResponse(socket,
                          200,
                          QByteArrayLiteral("OK"),
                          buildHttpSuccessEnvelope(
                              plasma_bridge::toJsonObject(plasma_bridge::toAudioInputState(m_audioStateStore->audioState()))));
        return;
    case SnapshotRoute::DefaultSink: {
        if (m_audioStateStore == nullptr || !m_audioStateStore->isReady()) {
            writeJsonErrorResponse(socket, 503, QByteArrayLiteral("Service Unavailable"), QStringLiteral("not_ready"),
                                   QStringLiteral("Initial audio state is not ready yet."));
            return;
        }

        const std::optional<plasma_bridge::AudioSinkState> defaultSink = m_audioStateStore->defaultSink();
        if (!defaultSink.has_value()) {
            writeJsonErrorResponse(socket, 404, QByteArrayLiteral("Not Found"), QStringLiteral("not_found"),
                                   QStringLiteral("No default audio sink available."));
            return;
        }

        writeJsonResponse(socket,
                          200,
                          QByteArrayLiteral("OK"),
                          buildHttpSuccessEnvelope(plasma_bridge::toJsonObject(*defaultSink)));
        return;
    }
    case SnapshotRoute::DefaultSource: {
        if (m_audioStateStore == nullptr || !m_audioStateStore->isReady()) {
            writeJsonErrorResponse(socket, 503, QByteArrayLiteral("Service Unavailable"), QStringLiteral("not_ready"),
                                   QStringLiteral("Initial audio state is not ready yet."));
            return;
        }

        const std::optional<plasma_bridge::AudioSourceState> defaultSource = m_audioStateStore->defaultSource();
        if (!defaultSource.has_value()) {
            writeJsonErrorResponse(socket, 404, QByteArrayLiteral("Not Found"), QStringLiteral("not_found"),
                                   QStringLiteral("No default audio source available."));
            return;
        }

        writeJsonResponse(socket,
                          200,
                          QByteArrayLiteral("OK"),
                          buildHttpSuccessEnvelope(plasma_bridge::toJsonObject(*defaultSource)));
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

    QList<QPair<QByteArray, QByteArray>> responseHeaders = m_corsHeaders.take(socket);
    responseHeaders.append(extraHeaders);

    socket->write(buildHttpResponse(statusCode, reasonPhrase, contentType, body, responseHeaders));
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
                                                const QList<QPair<QByteArray, QByteArray>> &extraHeaders,
                                                QJsonObject details)
{
    writeJsonResponse(socket, statusCode, reasonPhrase, buildHttpErrorEnvelope(code, message, details), extraHeaders);
}

} // namespace plasma_bridge::api
