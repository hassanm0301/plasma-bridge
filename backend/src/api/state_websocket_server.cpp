#include "api/state_websocket_server.h"

#include "api/json_envelope.h"
#include "common/audio_state.h"
#include "common/media_state.h"
#include "common/window_state.h"
#include "state/audio_state_store.h"
#include "state/media_state_store.h"
#include "state/window_state_store.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStringList>
#include <QTimer>
#include <QWebSocket>

namespace plasma_bridge::api
{
namespace
{

QString requestPath(const QWebSocket *socket)
{
    if (socket == nullptr) {
        return QStringLiteral("/");
    }

    const QString path = socket->requestUrl().path();
    return path.isEmpty() ? QStringLiteral("/") : path;
}

QJsonValue stringOrNull(const QString &value)
{
    return value.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(value);
}

QJsonObject buildFullStatePayload(const state::AudioStateStore *audioStateStore,
                                  const state::MediaStateStore *mediaStateStore,
                                  const state::WindowStateStore *windowStateStore)
{
    QJsonObject payload;
    if (audioStateStore != nullptr && audioStateStore->isReady()) {
        payload[QStringLiteral("audio")] = plasma_bridge::toJsonObject(audioStateStore->audioState());
    }
    if (mediaStateStore != nullptr && mediaStateStore->isReady()) {
        payload[QStringLiteral("media")] = plasma_bridge::toJsonObject(mediaStateStore->mediaState());
    }
    if (windowStateStore != nullptr && windowStateStore->isReady()) {
        payload[QStringLiteral("windowState")] = plasma_bridge::toJsonObject(windowStateStore->windowState());
    }
    return payload;
}

QJsonObject buildAudioPatchMessage(const QString &reason,
                                   const QString &sinkId,
                                   const QString &sourceId,
                                   const plasma_bridge::AudioState &state)
{
    QJsonObject change;
    change[QStringLiteral("path")] = QStringLiteral("audio");
    change[QStringLiteral("value")] = plasma_bridge::toJsonObject(state);

    QJsonArray changes;
    changes.append(change);

    QJsonObject payload;
    payload[QStringLiteral("reason")] = stringOrNull(reason);
    payload[QStringLiteral("sinkId")] = stringOrNull(sinkId);
    payload[QStringLiteral("sourceId")] = stringOrNull(sourceId);
    payload[QStringLiteral("playerId")] = QJsonValue(QJsonValue::Null);
    payload[QStringLiteral("windowId")] = QJsonValue(QJsonValue::Null);
    payload[QStringLiteral("changes")] = changes;
    return buildWebSocketSuccessEnvelope(QStringLiteral("patch"), payload);
}

QJsonObject buildMediaPatchMessage(const QString &reason,
                                   const QString &playerId,
                                   const plasma_bridge::MediaState &state)
{
    QJsonObject change;
    change[QStringLiteral("path")] = QStringLiteral("media");
    change[QStringLiteral("value")] = plasma_bridge::toJsonObject(state);

    QJsonArray changes;
    changes.append(change);

    QJsonObject payload;
    payload[QStringLiteral("reason")] = stringOrNull(reason);
    payload[QStringLiteral("sinkId")] = QJsonValue(QJsonValue::Null);
    payload[QStringLiteral("sourceId")] = QJsonValue(QJsonValue::Null);
    payload[QStringLiteral("playerId")] = stringOrNull(playerId);
    payload[QStringLiteral("windowId")] = QJsonValue(QJsonValue::Null);
    payload[QStringLiteral("changes")] = changes;
    return buildWebSocketSuccessEnvelope(QStringLiteral("patch"), payload);
}

QJsonObject buildWindowPatchMessage(const QString &reason,
                                    const QString &windowId,
                                    const plasma_bridge::WindowSnapshot &state)
{
    QJsonObject change;
    change[QStringLiteral("path")] = QStringLiteral("windowState");
    change[QStringLiteral("value")] = plasma_bridge::toJsonObject(state);

    QJsonArray changes;
    changes.append(change);

    QJsonObject payload;
    payload[QStringLiteral("reason")] = stringOrNull(reason);
    payload[QStringLiteral("sinkId")] = QJsonValue(QJsonValue::Null);
    payload[QStringLiteral("sourceId")] = QJsonValue(QJsonValue::Null);
    payload[QStringLiteral("playerId")] = QJsonValue(QJsonValue::Null);
    payload[QStringLiteral("windowId")] = stringOrNull(windowId);
    payload[QStringLiteral("changes")] = changes;
    return buildWebSocketSuccessEnvelope(QStringLiteral("patch"), payload);
}

QJsonObject buildErrorMessage(const QString &code, const QString &message)
{
    return buildWebSocketErrorEnvelope(code, message);
}

} // namespace

StateWebSocketServer::StateWebSocketServer(state::AudioStateStore *audioStateStore,
                                           state::WindowStateStore *windowStateStore,
                                           QObject *parent)
    : StateWebSocketServer(audioStateStore, nullptr, windowStateStore, parent)
{
}

StateWebSocketServer::StateWebSocketServer(state::AudioStateStore *audioStateStore,
                                           state::MediaStateStore *mediaStateStore,
                                           state::WindowStateStore *windowStateStore,
                                           QObject *parent)
    : QObject(parent)
    , m_audioStateStore(audioStateStore)
    , m_mediaStateStore(mediaStateStore)
    , m_windowStateStore(windowStateStore)
    , m_server(QStringLiteral("plasma_bridge_state"), QWebSocketServer::NonSecureMode)
{
    connect(&m_server, &QWebSocketServer::newConnection, this, &StateWebSocketServer::handleNewConnection);

    if (m_audioStateStore != nullptr) {
        connect(m_audioStateStore,
                &state::AudioStateStore::audioStateChanged,
                this,
                &StateWebSocketServer::handleAudioStateChanged);
    }

    if (m_mediaStateStore != nullptr) {
        connect(m_mediaStateStore,
                &state::MediaStateStore::mediaStateChanged,
                this,
                &StateWebSocketServer::handleMediaStateChanged);
    }

    if (m_windowStateStore != nullptr) {
        connect(m_windowStateStore,
                &state::WindowStateStore::windowStateChanged,
                this,
                &StateWebSocketServer::handleWindowStateChanged);
    }
}

bool StateWebSocketServer::listen(const QHostAddress &address, quint16 port)
{
    return m_server.listen(address, port);
}

QString StateWebSocketServer::errorString() const
{
    return m_server.errorString();
}

QHostAddress StateWebSocketServer::serverAddress() const
{
    return m_server.serverAddress();
}

quint16 StateWebSocketServer::serverPort() const
{
    return m_server.serverPort();
}

QString StateWebSocketServer::endpointPath()
{
    return QString::fromUtf8(kStateWebSocketPath);
}

int StateWebSocketServer::protocolVersion()
{
    return kStateWebSocketProtocolVersion;
}

void StateWebSocketServer::handleNewConnection()
{
    while (QWebSocket *socket = m_server.nextPendingConnection()) {
        connect(socket, &QWebSocket::disconnected, socket, &QWebSocket::deleteLater);

        if (requestPath(socket) != endpointPath()) {
            sendError(socket,
                      QStringLiteral("unknown_endpoint"),
                      QStringLiteral("Unknown WebSocket endpoint \"%1\". Connect to %2.")
                          .arg(requestPath(socket), endpointPath()),
                      true);
            continue;
        }

        m_clients.insert(socket, ClientSession{false, false});

        connect(socket, &QWebSocket::textMessageReceived, this, [this, socket](const QString &message) {
            handleTextMessage(socket, message);
        });
        connect(socket, &QWebSocket::binaryMessageReceived, this, [this, socket](const QByteArray &) {
            handleBinaryMessage(socket);
        });
        connect(socket, &QWebSocket::disconnected, this, [this, socket]() {
            m_clients.remove(socket);
        });
    }
}

void StateWebSocketServer::handleTextMessage(QWebSocket *socket, const QString &message)
{
    if (socket == nullptr || !m_clients.contains(socket)) {
        return;
    }

    ClientSession &session = m_clients[socket];
    if (session.helloReceived) {
        sendError(socket,
                  QStringLiteral("protocol_error"),
                  QStringLiteral("Only the initial hello message is supported from clients."),
                  true);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        sendError(socket, QStringLiteral("bad_request"), QStringLiteral("Malformed JSON message."), true);
        return;
    }

    const QJsonObject object = document.object();
    if (object.value(QStringLiteral("type")).toString() != QStringLiteral("hello")
        || !object.value(QStringLiteral("payload")).isObject()
        || !object.value(QStringLiteral("error")).isNull()) {
        sendError(socket,
                  QStringLiteral("bad_request"),
                  QStringLiteral("Expected a hello message with payload.protocolVersion as the first client message."),
                  true);
        return;
    }

    const QJsonObject payload = object.value(QStringLiteral("payload")).toObject();
    if (payload.value(QStringLiteral("protocolVersion")).toInt(-1) != protocolVersion()) {
        sendError(socket,
                  QStringLiteral("unsupported_protocol_version"),
                  QStringLiteral("Only protocolVersion %1 is supported.").arg(QString::number(protocolVersion())),
                  true);
        return;
    }

    session.helloReceived = true;

    if (!isConfiguredStateReady()) {
        sendError(socket, QStringLiteral("not_ready"), notReadyMessage(), false);
        return;
    }

    sendFullState(socket);
}

void StateWebSocketServer::handleBinaryMessage(QWebSocket *socket)
{
    if (socket == nullptr || !m_clients.contains(socket)) {
        return;
    }

    sendError(socket,
              QStringLiteral("bad_request"),
              QStringLiteral("Binary WebSocket messages are not supported."),
              true);
}

void StateWebSocketServer::handleAudioStateChanged(const QString &reason, const QString &sinkId, const QString &sourceId)
{
    if (m_audioStateStore == nullptr || !m_audioStateStore->isReady()) {
        return;
    }

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        QWebSocket *socket = it.key();
        ClientSession &session = it.value();

        if (!session.helloReceived) {
            continue;
        }

        if (!session.fullStateSent) {
            if (!isConfiguredStateReady()) {
                continue;
            }
            sendFullState(socket);
            continue;
        }

        sendAudioPatch(socket, reason, sinkId, sourceId);
    }
}

void StateWebSocketServer::handleMediaStateChanged(const QString &reason, const QString &playerId)
{
    if (m_mediaStateStore == nullptr || !m_mediaStateStore->isReady()) {
        return;
    }

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        QWebSocket *socket = it.key();
        ClientSession &session = it.value();

        if (!session.helloReceived) {
            continue;
        }

        if (!session.fullStateSent) {
            if (!isConfiguredStateReady()) {
                continue;
            }
            sendFullState(socket);
            continue;
        }

        sendMediaPatch(socket, reason, playerId);
    }
}

void StateWebSocketServer::handleWindowStateChanged(const QString &reason, const QString &windowId)
{
    if (m_windowStateStore == nullptr || !m_windowStateStore->isReady()) {
        return;
    }

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        QWebSocket *socket = it.key();
        ClientSession &session = it.value();

        if (!session.helloReceived) {
            continue;
        }

        if (!session.fullStateSent) {
            if (!isConfiguredStateReady()) {
                continue;
            }
            sendFullState(socket);
            continue;
        }

        sendWindowPatch(socket, reason, windowId);
    }
}

bool StateWebSocketServer::isConfiguredStateReady() const
{
    const bool hasAudioState = m_audioStateStore != nullptr;
    const bool hasMediaState = m_mediaStateStore != nullptr;
    const bool hasWindowState = m_windowStateStore != nullptr;
    if (!hasAudioState && !hasMediaState && !hasWindowState) {
        return false;
    }

    return (!hasAudioState || m_audioStateStore->isReady()) && (!hasMediaState || m_mediaStateStore->isReady())
        && (!hasWindowState || m_windowStateStore->isReady());
}

QString StateWebSocketServer::notReadyMessage() const
{
    const bool needsAudio = m_audioStateStore != nullptr && !m_audioStateStore->isReady();
    const bool needsMedia = m_mediaStateStore != nullptr && !m_mediaStateStore->isReady();
    const bool needsWindow = m_windowStateStore != nullptr && !m_windowStateStore->isReady();

    QStringList missingStates;
    if (needsAudio) {
        missingStates.append(QStringLiteral("audio"));
    }
    if (needsMedia) {
        missingStates.append(QStringLiteral("media"));
    }
    if (needsWindow) {
        missingStates.append(QStringLiteral("window"));
    }

    if (missingStates.isEmpty()) {
        return QStringLiteral("Initial state is not ready yet.");
    }
    if (missingStates.size() == 1) {
        return QStringLiteral("Initial %1 state is not ready yet.").arg(missingStates.first());
    }
    if (missingStates.size() == 2) {
        return QStringLiteral("Initial %1 and %2 state are not ready yet.").arg(missingStates.at(0), missingStates.at(1));
    }

    return QStringLiteral("Initial %1, %2, and %3 state are not ready yet.")
        .arg(missingStates.at(0), missingStates.at(1), missingStates.at(2));
}

void StateWebSocketServer::sendFullState(QWebSocket *socket)
{
    if (socket == nullptr || !m_clients.contains(socket)) {
        return;
    }

    if (!isConfiguredStateReady()) {
        return;
    }

    ClientSession &session = m_clients[socket];
    socket->sendTextMessage(QJsonDocument(buildWebSocketSuccessEnvelope(
                              QStringLiteral("fullState"),
                              buildFullStatePayload(m_audioStateStore, m_mediaStateStore, m_windowStateStore)))
                                .toJson(QJsonDocument::Compact));
    session.fullStateSent = true;
}

void StateWebSocketServer::sendAudioPatch(QWebSocket *socket,
                                          const QString &reason,
                                          const QString &sinkId,
                                          const QString &sourceId)
{
    if (socket == nullptr || !m_clients.contains(socket) || m_audioStateStore == nullptr || !m_audioStateStore->isReady()) {
        return;
    }

    socket->sendTextMessage(QJsonDocument(buildAudioPatchMessage(reason, sinkId, sourceId, m_audioStateStore->audioState()))
                                .toJson(QJsonDocument::Compact));
}

void StateWebSocketServer::sendMediaPatch(QWebSocket *socket, const QString &reason, const QString &playerId)
{
    if (socket == nullptr || !m_clients.contains(socket) || m_mediaStateStore == nullptr || !m_mediaStateStore->isReady()) {
        return;
    }

    socket->sendTextMessage(QJsonDocument(buildMediaPatchMessage(reason, playerId, m_mediaStateStore->mediaState()))
                                .toJson(QJsonDocument::Compact));
}

void StateWebSocketServer::sendWindowPatch(QWebSocket *socket, const QString &reason, const QString &windowId)
{
    if (socket == nullptr || !m_clients.contains(socket) || m_windowStateStore == nullptr || !m_windowStateStore->isReady()) {
        return;
    }

    socket->sendTextMessage(QJsonDocument(buildWindowPatchMessage(reason, windowId, m_windowStateStore->windowState()))
                                .toJson(QJsonDocument::Compact));
}

void StateWebSocketServer::sendError(QWebSocket *socket, const QString &code, const QString &message, bool closeAfter)
{
    if (socket == nullptr) {
        return;
    }

    socket->sendTextMessage(QJsonDocument(buildErrorMessage(code, message)).toJson(QJsonDocument::Compact));

    if (closeAfter) {
        QTimer::singleShot(0, socket, [socket]() {
            socket->close();
        });
    }
}

} // namespace plasma_bridge::api
