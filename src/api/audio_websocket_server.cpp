#include "api/audio_websocket_server.h"

#include "api/json_envelope.h"
#include "common/audio_state.h"
#include "state/audio_state_store.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
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

QJsonObject buildAudioPayload(const plasma_bridge::AudioState &state)
{
    QJsonObject payload;
    payload[QStringLiteral("audio")] = plasma_bridge::toJsonObject(state);
    return payload;
}

QJsonObject buildFullStateMessage(const plasma_bridge::AudioState &state)
{
    return buildWebSocketSuccessEnvelope(QStringLiteral("fullState"), buildAudioPayload(state));
}

QJsonObject buildPatchMessage(const QString &reason,
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
    payload[QStringLiteral("changes")] = changes;
    return buildWebSocketSuccessEnvelope(QStringLiteral("patch"), payload);
}

QJsonObject buildErrorMessage(const QString &code, const QString &message)
{
    return buildWebSocketErrorEnvelope(code, message);
}

} // namespace

AudioWebSocketServer::AudioWebSocketServer(state::AudioStateStore *audioStateStore, QObject *parent)
    : QObject(parent)
    , m_audioStateStore(audioStateStore)
    , m_server(QStringLiteral("plasma_bridge_audio"), QWebSocketServer::NonSecureMode)
{
    connect(&m_server, &QWebSocketServer::newConnection, this, &AudioWebSocketServer::handleNewConnection);

    if (m_audioStateStore != nullptr) {
        connect(m_audioStateStore,
                &state::AudioStateStore::audioStateChanged,
                this,
                &AudioWebSocketServer::handleStateChanged);
    }
}

bool AudioWebSocketServer::listen(const QHostAddress &address, quint16 port)
{
    return m_server.listen(address, port);
}

QString AudioWebSocketServer::errorString() const
{
    return m_server.errorString();
}

QHostAddress AudioWebSocketServer::serverAddress() const
{
    return m_server.serverAddress();
}

quint16 AudioWebSocketServer::serverPort() const
{
    return m_server.serverPort();
}

QString AudioWebSocketServer::endpointPath()
{
    return QString::fromUtf8(kAudioWebSocketPath);
}

int AudioWebSocketServer::protocolVersion()
{
    return kAudioWebSocketProtocolVersion;
}

void AudioWebSocketServer::handleNewConnection()
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

        m_clients.insert(socket, ClientSession{});

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

void AudioWebSocketServer::handleTextMessage(QWebSocket *socket, const QString &message)
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

    if (m_audioStateStore == nullptr || !m_audioStateStore->isReady()) {
        sendError(socket,
                  QStringLiteral("not_ready"),
                  QStringLiteral("Initial audio state is not ready yet."),
                  false);
        return;
    }

    sendFullState(socket);
}

void AudioWebSocketServer::handleBinaryMessage(QWebSocket *socket)
{
    if (socket == nullptr || !m_clients.contains(socket)) {
        return;
    }

    sendError(socket,
              QStringLiteral("bad_request"),
              QStringLiteral("Binary WebSocket messages are not supported."),
              true);
}

void AudioWebSocketServer::handleStateChanged(const QString &reason, const QString &sinkId, const QString &sourceId)
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
            sendFullState(socket);
            continue;
        }

        sendPatch(socket, reason, sinkId, sourceId);
    }
}

void AudioWebSocketServer::sendFullState(QWebSocket *socket)
{
    if (socket == nullptr || !m_clients.contains(socket) || m_audioStateStore == nullptr || !m_audioStateStore->isReady()) {
        return;
    }

    socket->sendTextMessage(QJsonDocument(buildFullStateMessage(m_audioStateStore->audioState())).toJson(QJsonDocument::Compact));
    m_clients[socket].fullStateSent = true;
}

void AudioWebSocketServer::sendPatch(QWebSocket *socket,
                                     const QString &reason,
                                     const QString &sinkId,
                                     const QString &sourceId)
{
    if (socket == nullptr || !m_clients.contains(socket) || m_audioStateStore == nullptr || !m_audioStateStore->isReady()) {
        return;
    }

    socket->sendTextMessage(QJsonDocument(buildPatchMessage(reason, sinkId, sourceId, m_audioStateStore->audioState()))
                                .toJson(QJsonDocument::Compact));
}

void AudioWebSocketServer::sendError(QWebSocket *socket, const QString &code, const QString &message, bool closeAfter)
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
