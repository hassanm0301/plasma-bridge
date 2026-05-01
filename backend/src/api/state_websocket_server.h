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
class MediaStateStore;
class WindowStateStore;
}

namespace plasma_bridge::api
{

inline constexpr auto kStateWebSocketPath = PLASMA_BRIDGE_WS_PATH;
inline constexpr int kStateWebSocketProtocolVersion = PLASMA_BRIDGE_WS_PROTOCOL_VERSION;

class StateWebSocketServer final : public QObject
{
    Q_OBJECT

public:
    explicit StateWebSocketServer(state::AudioStateStore *audioStateStore,
                                  state::WindowStateStore *windowStateStore = nullptr,
                                  QObject *parent = nullptr);
    explicit StateWebSocketServer(state::AudioStateStore *audioStateStore,
                                  state::MediaStateStore *mediaStateStore,
                                  state::WindowStateStore *windowStateStore = nullptr,
                                  QObject *parent = nullptr);

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
    void handleAudioStateChanged(const QString &reason, const QString &sinkId, const QString &sourceId);
    void handleMediaStateChanged(const QString &reason, const QString &playerId);
    void handleWindowStateChanged(const QString &reason, const QString &windowId);
    bool isConfiguredStateReady() const;
    QString notReadyMessage() const;
    void sendFullState(QWebSocket *socket);
    void sendAudioPatch(QWebSocket *socket, const QString &reason, const QString &sinkId, const QString &sourceId);
    void sendMediaPatch(QWebSocket *socket, const QString &reason, const QString &playerId);
    void sendWindowPatch(QWebSocket *socket, const QString &reason, const QString &windowId);
    void sendError(QWebSocket *socket, const QString &code, const QString &message, bool closeAfter);

    state::AudioStateStore *m_audioStateStore = nullptr;
    state::MediaStateStore *m_mediaStateStore = nullptr;
    state::WindowStateStore *m_windowStateStore = nullptr;
    QHash<QWebSocket *, ClientSession> m_clients;
    QWebSocketServer m_server;
};

} // namespace plasma_bridge::api
