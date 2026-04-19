#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QtTest>

#include <cstring>

#define private public
#include "api/snapshot_http_server.h"
#undef private
#include "state/audio_state_store.h"
#include "tests/support/test_support.h"

class SnapshotHttpServerFeatureTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void servesSnapshotAndDefaultSinkEndpoints();
    void reportsNotReadyAndMissingDefaultSink();
    void handlesMethodNotAllowedAndUnknownPath();
    void rejectsMalformedAndOversizedRequests();
    void servesDocsAndRewritesSpecHosts();

private:
    static QHostAddress bindAddress();
    static QByteArray readReplyBody(QNetworkReply *reply);
};

class InMemoryTcpSocket final : public QTcpSocket
{
public:
    explicit InMemoryTcpSocket(QByteArray incomingData = {})
        : m_incomingData(std::move(incomingData))
    {
        open(QIODevice::ReadWrite);
    }

    const QByteArray &writtenData() const
    {
        return m_writtenData;
    }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        const qint64 remaining = m_incomingData.size() - m_readOffset;
        const qint64 bytesToRead = qMin(maxSize, remaining);
        if (bytesToRead <= 0) {
            return -1;
        }

        memcpy(data, m_incomingData.constData() + m_readOffset, bytesToRead);
        m_readOffset += bytesToRead;
        return bytesToRead;
    }

    qint64 writeData(const char *data, qint64 len) override
    {
        m_writtenData.append(data, len);
        return len;
    }

private:
    QByteArray m_incomingData;
    qint64 m_readOffset = 0;
    QByteArray m_writtenData;
};

void SnapshotHttpServerFeatureTest::initTestCase()
{
    plasma_bridge::tests::ensureDocsResourcesInitialized();
}

void SnapshotHttpServerFeatureTest::servesSnapshotAndDefaultSinkEndpoints()
{
    plasma_bridge::state::AudioStateStore store;
    const plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    store.updateAudioState(state, true, QStringLiteral("initial"));

    plasma_bridge::api::SnapshotHttpServer server(&store, QStringLiteral("127.0.0.1"), 18080, 18081);
    QVERIFY(server.listen(bindAddress(), 0));

    QNetworkAccessManager manager;

    QNetworkReply *sinksReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                                          QStringLiteral("/snapshot/audio/sinks"))));
    QSignalSpy sinksSpy(sinksReply, &QNetworkReply::finished);
    QVERIFY(sinksSpy.wait());
    QCOMPARE(sinksReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    const QJsonObject sinksJson = plasma_bridge::tests::parseJsonObject(readReplyBody(sinksReply));
    QCOMPARE(sinksJson.value(QStringLiteral("selectedSinkId")).toString(), state.selectedSinkId);
    QCOMPARE(sinksJson.value(QStringLiteral("sinks")).toArray().size(), state.sinks.size());
    sinksReply->deleteLater();

    QNetworkReply *defaultReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                                            QStringLiteral("/snapshot/audio/default-sink"))));
    QSignalSpy defaultSpy(defaultReply, &QNetworkReply::finished);
    QVERIFY(defaultSpy.wait());
    QCOMPARE(defaultReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    const QJsonObject defaultJson = plasma_bridge::tests::parseJsonObject(readReplyBody(defaultReply));
    QCOMPARE(defaultJson.value(QStringLiteral("id")).toString(), state.selectedSinkId);
    defaultReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::reportsNotReadyAndMissingDefaultSink()
{
    QNetworkAccessManager manager;

    plasma_bridge::state::AudioStateStore notReadyStore;
    plasma_bridge::api::SnapshotHttpServer notReadyServer(&notReadyStore, QStringLiteral("127.0.0.1"), 18080, 18081);
    QVERIFY(notReadyServer.listen(bindAddress(), 0));

    QNetworkReply *notReadyReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(notReadyServer.serverPort(),
                                                                                             QStringLiteral("/snapshot/audio/default-sink"))));
    QSignalSpy notReadySpy(notReadyReply, &QNetworkReply::finished);
    QVERIFY(notReadySpy.wait());
    QCOMPARE(notReadyReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 503);
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(notReadyReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("not_ready"));
    notReadyReply->deleteLater();

    plasma_bridge::state::AudioStateStore missingDefaultStore;
    plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    state.selectedSinkId.clear();
    state.sinks[0].isDefault = false;
    state.sinks[1].isDefault = false;
    missingDefaultStore.updateAudioState(state, true, QStringLiteral("initial"));

    plasma_bridge::api::SnapshotHttpServer missingDefaultServer(&missingDefaultStore, QStringLiteral("127.0.0.1"), 18080, 18081);
    QVERIFY(missingDefaultServer.listen(bindAddress(), 0));

    QNetworkReply *missingReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(missingDefaultServer.serverPort(),
                                                                                            QStringLiteral("/snapshot/audio/default-sink"))));
    QSignalSpy missingSpy(missingReply, &QNetworkReply::finished);
    QVERIFY(missingSpy.wait());
    QCOMPARE(missingReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 404);
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(missingReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("not_found"));
    missingReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::handlesMethodNotAllowedAndUnknownPath()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::api::SnapshotHttpServer server(&store, QStringLiteral("127.0.0.1"), 18080, 18081);
    QVERIFY(server.listen(bindAddress(), 0));

    QNetworkAccessManager manager;

    QNetworkRequest request(plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/snapshot/audio/sinks")));
    QNetworkReply *postReply = manager.sendCustomRequest(request, "POST");
    QSignalSpy postSpy(postReply, &QNetworkReply::finished);
    QVERIFY(postSpy.wait());
    QCOMPARE(postReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 405);
    QCOMPARE(postReply->rawHeader("Allow"), QByteArrayLiteral("GET"));
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(postReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("method_not_allowed"));
    postReply->deleteLater();

    QNetworkReply *notFoundReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                                             QStringLiteral("/unknown"))));
    QSignalSpy notFoundSpy(notFoundReply, &QNetworkReply::finished);
    QVERIFY(notFoundSpy.wait());
    QCOMPARE(notFoundReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 404);
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(notFoundReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("not_found"));
    notFoundReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::rejectsMalformedAndOversizedRequests()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::api::SnapshotHttpServer server(&store, QStringLiteral("127.0.0.1"), 18080, 18081);
    QVERIFY(server.listen(bindAddress(), 0));

    InMemoryTcpSocket malformedSocket;
    server.processRequest(&malformedSocket, QByteArrayLiteral("GET /snapshot/audio/sinks"));
    QVERIFY(malformedSocket.writtenData().startsWith("HTTP/1.1 400 Bad Request"));
    QVERIFY(malformedSocket.writtenData().contains("\"code\":\"bad_request\""));

    QByteArray oversizedRequest = "GET /snapshot/audio/sinks HTTP/1.1\r\nHost: 127.0.0.1\r\nX-Fill: ";
    oversizedRequest += QByteArray(20000, 'a');
    oversizedRequest += "\r\n\r\n";
    InMemoryTcpSocket oversizedSocket(oversizedRequest);
    server.handleReadyRead(&oversizedSocket);
    QVERIFY(oversizedSocket.writtenData().startsWith("HTTP/1.1 400 Bad Request"));
    QVERIFY(oversizedSocket.writtenData().contains("\"message\":\"Request is too large.\""));
}

void SnapshotHttpServerFeatureTest::servesDocsAndRewritesSpecHosts()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::api::SnapshotHttpServer server(&store, QStringLiteral("localhost"), 19080, 19081);
    QVERIFY(server.listen(bindAddress(), 0));

    QNetworkAccessManager manager;

    QNetworkRequest redirectRequest(plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/docs")));
    redirectRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    QNetworkReply *redirectReply = manager.get(redirectRequest);
    QSignalSpy redirectSpy(redirectReply, &QNetworkReply::finished);
    QVERIFY(redirectSpy.wait());
    QCOMPARE(redirectReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 301);
    QCOMPARE(redirectReply->rawHeader("Location"), QByteArrayLiteral("/docs/"));
    redirectReply->deleteLater();

    const QStringList docPaths = {
        QStringLiteral("/docs/"),
        QStringLiteral("/docs/http"),
        QStringLiteral("/docs/ws"),
    };

    for (const QString &path : docPaths) {
        QNetworkReply *reply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(), path)));
        QSignalSpy replySpy(reply, &QNetworkReply::finished);
        QVERIFY(replySpy.wait());
        QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
        QVERIFY(!readReplyBody(reply).isEmpty());
        reply->deleteLater();
    }

    QNetworkReply *openApiReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                                            QStringLiteral("/docs/openapi.yaml"))));
    QSignalSpy openApiSpy(openApiReply, &QNetworkReply::finished);
    QVERIFY(openApiSpy.wait());
    QCOMPARE(openApiReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    const QString openApiBody = QString::fromUtf8(readReplyBody(openApiReply));
    QVERIFY(openApiBody.contains(QStringLiteral("http://localhost:19080")));
    openApiReply->deleteLater();

    QNetworkReply *asyncApiReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                                             QStringLiteral("/docs/asyncapi.yaml"))));
    QSignalSpy asyncApiSpy(asyncApiReply, &QNetworkReply::finished);
    QVERIFY(asyncApiSpy.wait());
    QCOMPARE(asyncApiReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    const QString asyncApiBody = QString::fromUtf8(readReplyBody(asyncApiReply));
    QVERIFY(asyncApiBody.contains(QStringLiteral("localhost:19081")));
    asyncApiReply->deleteLater();
}


QHostAddress SnapshotHttpServerFeatureTest::bindAddress()
{
    return QHostAddress(QStringLiteral("127.0.0.1"));
}

QByteArray SnapshotHttpServerFeatureTest::readReplyBody(QNetworkReply *reply)
{
    return reply == nullptr ? QByteArray() : reply->readAll();
}

QTEST_GUILESS_MAIN(SnapshotHttpServerFeatureTest)

#include "test_snapshot_http_server.moc"
