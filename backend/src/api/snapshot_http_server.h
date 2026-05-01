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
class MediaController;
class WindowActivationController;
}

namespace plasma_bridge::state
{
class AudioStateStore;
class MediaStateStore;
class WindowStateStore;
}

namespace plasma_bridge::api
{

struct AllowedOrigin {
    QString scheme;
    QString host;
    quint16 port = 0;
};

class SnapshotHttpServer final : public QObject
{
    Q_OBJECT

public:
    static bool parseAllowedOrigin(const QString &value, AllowedOrigin *outOrigin, QString *errorMessage = nullptr);

    explicit SnapshotHttpServer(state::AudioStateStore *audioStateStore,
                                control::AudioVolumeController *audioVolumeController,
                                control::AudioDeviceController *audioDeviceController,
                                const QString &documentationHost,
                                quint16 documentationHttpPort,
                                quint16 documentationWsPort,
                                const QList<AllowedOrigin> &allowedOrigins = {},
                                QObject *parent = nullptr);
    explicit SnapshotHttpServer(state::AudioStateStore *audioStateStore,
                                state::MediaStateStore *mediaStateStore,
                                control::AudioVolumeController *audioVolumeController,
                                control::AudioDeviceController *audioDeviceController,
                                control::MediaController *mediaController,
                                const QString &documentationHost,
                                quint16 documentationHttpPort,
                                quint16 documentationWsPort,
                                const QList<AllowedOrigin> &allowedOrigins = {},
                                QObject *parent = nullptr);
    explicit SnapshotHttpServer(state::AudioStateStore *audioStateStore,
                                state::WindowStateStore *windowStateStore,
                                control::AudioVolumeController *audioVolumeController,
                                control::AudioDeviceController *audioDeviceController,
                                control::WindowActivationController *windowActivationController,
                                const QString &documentationHost,
                                quint16 documentationHttpPort,
                                quint16 documentationWsPort,
                                const QList<AllowedOrigin> &allowedOrigins = {},
                                QObject *parent = nullptr);
    explicit SnapshotHttpServer(state::AudioStateStore *audioStateStore,
                                state::MediaStateStore *mediaStateStore,
                                state::WindowStateStore *windowStateStore,
                                control::AudioVolumeController *audioVolumeController,
                                control::AudioDeviceController *audioDeviceController,
                                control::MediaController *mediaController,
                                control::WindowActivationController *windowActivationController,
                                const QString &documentationHost,
                                quint16 documentationHttpPort,
                                quint16 documentationWsPort,
                                const QList<AllowedOrigin> &allowedOrigins = {},
                                QObject *parent = nullptr);
    explicit SnapshotHttpServer(state::AudioStateStore *audioStateStore,
                                state::WindowStateStore *windowStateStore,
                                control::AudioVolumeController *audioVolumeController,
                                control::AudioDeviceController *audioDeviceController,
                                const QString &documentationHost,
                                quint16 documentationHttpPort,
                                quint16 documentationWsPort,
                                const QList<AllowedOrigin> &allowedOrigins = {},
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
    state::MediaStateStore *m_mediaStateStore = nullptr;
    state::WindowStateStore *m_windowStateStore = nullptr;
    control::AudioVolumeController *m_audioVolumeController = nullptr;
    control::AudioDeviceController *m_audioDeviceController = nullptr;
    control::MediaController *m_mediaController = nullptr;
    control::WindowActivationController *m_windowActivationController = nullptr;
    QString m_documentationHost;
    quint16 m_documentationHttpPort = 0;
    quint16 m_documentationWsPort = 0;
    QList<AllowedOrigin> m_allowedOrigins;
    QHash<QTcpSocket *, QByteArray> m_pendingRequests;
    QHash<QTcpSocket *, QList<QPair<QByteArray, QByteArray>>> m_corsHeaders;
    QTcpServer m_server;
};

} // namespace plasma_bridge::api
