#pragma once

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

inline constexpr auto kAudioWebSocketPath = "/ws/audio";

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

private:
    struct ClientSession {
        bool helloReceived = false;
        bool fullStateSent = false;
    };

    void handleNewConnection();
    void handleTextMessage(QWebSocket *socket, const QString &message);
    void handleBinaryMessage(QWebSocket *socket);
    void handleStateChanged(const QString &reason, const QString &sinkId);
    void sendFullState(QWebSocket *socket);
    void sendPatch(QWebSocket *socket, const QString &reason, const QString &sinkId);
    void sendError(QWebSocket *socket, const QString &code, const QString &message, bool closeAfter);

    state::AudioStateStore *m_audioStateStore = nullptr;
    QWebSocketServer m_server;
    QHash<QWebSocket *, ClientSession> m_clients;
};

} // namespace plasma_bridge::api
