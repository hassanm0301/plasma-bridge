#pragma once

#include <QByteArray>
#include <QJsonDocument>
#include <QObject>
#include <QHash>
#include <QList>
#include <QPair>
#include <QTcpServer>

class QHostAddress;
class QTcpSocket;

namespace plasma_bridge::state
{
class AudioStateStore;
}

namespace plasma_bridge::api
{

class SnapshotHttpServer final : public QObject
{
    Q_OBJECT

public:
    explicit SnapshotHttpServer(state::AudioStateStore *audioStateStore,
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
                                const QList<QPair<QByteArray, QByteArray>> &extraHeaders = {});

    state::AudioStateStore *m_audioStateStore = nullptr;
    QString m_documentationHost;
    quint16 m_documentationHttpPort = 0;
    quint16 m_documentationWsPort = 0;
    QTcpServer m_server;
    QHash<QTcpSocket *, QByteArray> m_pendingRequests;
};

} // namespace plasma_bridge::api
