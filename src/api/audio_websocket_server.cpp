#include "api/audio_websocket_server.h"

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

constexpr int kProtocolVersion = 1;

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

QJsonObject buildAudioEnvelope(const plasma_bridge::AudioState &state)
{
    QJsonObject audioEnvelope;
    audioEnvelope[QStringLiteral("audio")] = plasma_bridge::toJsonObject(state);
    return audioEnvelope;
}

QJsonObject buildFullStateMessage(const plasma_bridge::AudioState &state)
{
    QJsonObject message;
    message[QStringLiteral("type")] = QStringLiteral("fullState");
    message[QStringLiteral("state")] = buildAudioEnvelope(state);
    return message;
}

QJsonObject buildPatchMessage(const QString &reason, const QString &sinkId, const plasma_bridge::AudioState &state)
{
    QJsonObject change;
    change[QStringLiteral("path")] = QStringLiteral("audio");
    change[QStringLiteral("value")] = plasma_bridge::toJsonObject(state);

    QJsonArray changes;
    changes.append(change);

    QJsonObject message;
    message[QStringLiteral("type")] = QStringLiteral("patch");
    message[QStringLiteral("reason")] = stringOrNull(reason);
    message[QStringLiteral("sinkId")] = stringOrNull(sinkId);
    message[QStringLiteral("changes")] = changes;
    return message;
}

QJsonObject buildErrorMessage(const QString &code, const QString &message)
{
    QJsonObject error;
    error[QStringLiteral("type")] = QStringLiteral("error");
    error[QStringLiteral("code")] = code;
    error[QStringLiteral("message")] = message;
    return error;
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
    if (object.value(QStringLiteral("type")).toString() != QStringLiteral("hello")) {
        sendError(socket,
                  QStringLiteral("bad_request"),
                  QStringLiteral("Expected a hello message as the first client message."),
                  true);
        return;
    }

    if (object.value(QStringLiteral("protocolVersion")).toInt(-1) != kProtocolVersion) {
        sendError(socket,
                  QStringLiteral("unsupported_protocol_version"),
                  QStringLiteral("Only protocolVersion 1 is supported."),
                  true);
        return;
    }

    session.helloReceived = true;

    if (m_audioStateStore == nullptr || !m_audioStateStore->isReady()) {
        sendError(socket,
                  QStringLiteral("not_ready"),
                  QStringLiteral("Initial audio sink state is not ready yet."),
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

void AudioWebSocketServer::handleStateChanged(const QString &reason, const QString &sinkId)
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

        sendPatch(socket, reason, sinkId);
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

void AudioWebSocketServer::sendPatch(QWebSocket *socket, const QString &reason, const QString &sinkId)
{
    if (socket == nullptr || !m_clients.contains(socket) || m_audioStateStore == nullptr || !m_audioStateStore->isReady()) {
        return;
    }

    socket->sendTextMessage(QJsonDocument(buildPatchMessage(reason, sinkId, m_audioStateStore->audioState()))
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
