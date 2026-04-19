#pragma once

#include "plasma_bridge_build_config.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QWebSocketServer>

class QHostAddress;
class QWebSocket;

namespace plasma_bridge::state
{
class AudioStateStore;
}

namespace plasma_bridge::api
{

inline constexpr auto kAudioWebSocketPath = PLASMA_BRIDGE_AUDIO_WS_PATH;
inline constexpr int kAudioWebSocketProtocolVersion = PLASMA_BRIDGE_AUDIO_PROTOCOL_VERSION;

class AudioWebSocketServer final : public QObject
{
    Q_OBJECT

public:
    explicit AudioWebSocketServer(state::AudioStateStore *audioStateStore, QObject *parent = nullptr);

    bool listen(const QHostAddress &address, quint16 port);
    QString errorString() const;
    QHostAddress serverAddress() const;
    quint16 serverPort() const;
    static QString endpointPath();
    static int protocolVersion();

private:
    struct ClientSession {
        bool helloReceived = false;
        bool fullStateSent = false;
    };

    void handleNewConnection();
    void handleTextMessage(QWebSocket *socket, const QString &message);
    void handleBinaryMessage(QWebSocket *socket);
    void handleStateChanged(const QString &reason, const QString &sinkId, const QString &sourceId);
    void sendFullState(QWebSocket *socket);
    void sendPatch(QWebSocket *socket, const QString &reason, const QString &sinkId, const QString &sourceId);
    void sendError(QWebSocket *socket, const QString &code, const QString &message, bool closeAfter);

    state::AudioStateStore *m_audioStateStore = nullptr;
    QHash<QWebSocket *, ClientSession> m_clients;
    QWebSocketServer m_server;
};

} // namespace plasma_bridge::api
