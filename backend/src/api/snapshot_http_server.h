#pragma once

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QHash>
#include <QList>
#include <QPair>
#include <QTcpServer>

class QHostAddress;
class QTcpSocket;

namespace plasma_bridge::control
{
class AudioDeviceController;
class AudioVolumeController;
class WindowActivationController;
}

namespace plasma_bridge::state
{
class AudioStateStore;
class WindowStateStore;
}

namespace plasma_bridge::api
{

class SnapshotHttpServer final : public QObject
{
    Q_OBJECT

public:
    explicit SnapshotHttpServer(state::AudioStateStore *audioStateStore,
                                control::AudioVolumeController *audioVolumeController,
                                control::AudioDeviceController *audioDeviceController,
                                const QString &documentationHost,
                                quint16 documentationHttpPort,
                                quint16 documentationWsPort,
                                QObject *parent = nullptr);
    explicit SnapshotHttpServer(state::AudioStateStore *audioStateStore,
                                state::WindowStateStore *windowStateStore,
                                control::AudioVolumeController *audioVolumeController,
                                control::AudioDeviceController *audioDeviceController,
                                control::WindowActivationController *windowActivationController,
                                const QString &documentationHost,
                                quint16 documentationHttpPort,
                                quint16 documentationWsPort,
                                QObject *parent = nullptr);
    explicit SnapshotHttpServer(state::AudioStateStore *audioStateStore,
                                state::WindowStateStore *windowStateStore,
                                control::AudioVolumeController *audioVolumeController,
                                control::AudioDeviceController *audioDeviceController,
                                const QString &documentationHost,
                                quint16 documentationHttpPort,
                                quint16 documentationWsPort,
                                QObject *parent = nullptr);

    bool listen(const QHostAddress &address, quint16 port);
    QString errorString() const;
    QHostAddress serverAddress() const;
    quint16 serverPort() const;

private:
    void handleNewConnection();
    void handleReadyRead(QTcpSocket *socket);
    void processRequest(QTcpSocket *socket, const QByteArray &requestData);
    void writeByteResponse(QTcpSocket *socket,
                           int statusCode,
                           const QByteArray &reasonPhrase,
                           const QByteArray &contentType,
                           const QByteArray &body,
                           const QList<QPair<QByteArray, QByteArray>> &extraHeaders = {});
    void writeJsonResponse(QTcpSocket *socket,
                           int statusCode,
                           const QByteArray &reasonPhrase,
                           const QJsonDocument &document,
                           const QList<QPair<QByteArray, QByteArray>> &extraHeaders = {});
    void writeJsonErrorResponse(QTcpSocket *socket,
                                int statusCode,
                                const QByteArray &reasonPhrase,
                                const QString &code,
                                const QString &message,
                                const QList<QPair<QByteArray, QByteArray>> &extraHeaders = {},
                                QJsonObject details = {});

    state::AudioStateStore *m_audioStateStore = nullptr;
    state::WindowStateStore *m_windowStateStore = nullptr;
    control::AudioVolumeController *m_audioVolumeController = nullptr;
    control::AudioDeviceController *m_audioDeviceController = nullptr;
    control::WindowActivationController *m_windowActivationController = nullptr;
    QString m_documentationHost;
    quint16 m_documentationHttpPort = 0;
    quint16 m_documentationWsPort = 0;
    QHash<QTcpSocket *, QByteArray> m_pendingRequests;
    QHash<QTcpSocket *, QList<QPair<QByteArray, QByteArray>>> m_corsHeaders;
    QTcpServer m_server;
};

} // namespace plasma_bridge::api
