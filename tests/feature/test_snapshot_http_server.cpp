#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
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
    void servesVolumeControlEndpoints();
    void mapsVolumeControlFailures();
    void handlesMethodNotAllowedAndUnknownPath();
    void rejectsInvalidVolumeControlRequests();
    void rejectsMalformedAndOversizedRequests();
    void servesDocsAndRewritesSpecHosts();

private:
    static QHostAddress bindAddress();
    static QByteArray readReplyBody(QNetworkReply *reply);
    static QNetworkReply *postBytes(QNetworkAccessManager *manager,
                                    const QUrl &url,
                                    const QByteArray &body,
                                    const QByteArray &contentType);
    static QNetworkReply *postJson(QNetworkAccessManager *manager,
                                   const QUrl &url,
                                   const QJsonObject &body,
                                   const QByteArray &contentType = QByteArrayLiteral("application/json"));
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

    plasma_bridge::api::SnapshotHttpServer server(&store, nullptr, QStringLiteral("127.0.0.1"), 18080, 18081);
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
    QVERIFY(!sinksJson.contains(QStringLiteral("sources")));
    sinksReply->deleteLater();

    QNetworkReply *defaultReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                                            QStringLiteral("/snapshot/audio/default-sink"))));
    QSignalSpy defaultSpy(defaultReply, &QNetworkReply::finished);
    QVERIFY(defaultSpy.wait());
    QCOMPARE(defaultReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    const QJsonObject defaultJson = plasma_bridge::tests::parseJsonObject(readReplyBody(defaultReply));
    QCOMPARE(defaultJson.value(QStringLiteral("id")).toString(), state.selectedSinkId);
    defaultReply->deleteLater();

    QNetworkReply *sourcesReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                                            QStringLiteral("/snapshot/audio/sources"))));
    QSignalSpy sourcesSpy(sourcesReply, &QNetworkReply::finished);
    QVERIFY(sourcesSpy.wait());
    QCOMPARE(sourcesReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    const QJsonObject sourcesJson = plasma_bridge::tests::parseJsonObject(readReplyBody(sourcesReply));
    QCOMPARE(sourcesJson.value(QStringLiteral("selectedSourceId")).toString(), state.selectedSourceId);
    QCOMPARE(sourcesJson.value(QStringLiteral("sources")).toArray().size(), state.sources.size());
    QVERIFY(!sourcesJson.contains(QStringLiteral("sinks")));
    sourcesReply->deleteLater();

    QNetworkReply *defaultSourceReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                                                  QStringLiteral("/snapshot/audio/default-source"))));
    QSignalSpy defaultSourceSpy(defaultSourceReply, &QNetworkReply::finished);
    QVERIFY(defaultSourceSpy.wait());
    QCOMPARE(defaultSourceReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    const QJsonObject defaultSourceJson = plasma_bridge::tests::parseJsonObject(readReplyBody(defaultSourceReply));
    QCOMPARE(defaultSourceJson.value(QStringLiteral("id")).toString(), state.selectedSourceId);
    defaultSourceReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::reportsNotReadyAndMissingDefaultSink()
{
    QNetworkAccessManager manager;

    plasma_bridge::state::AudioStateStore notReadyStore;
    plasma_bridge::api::SnapshotHttpServer notReadyServer(&notReadyStore, nullptr, QStringLiteral("127.0.0.1"), 18080, 18081);
    QVERIFY(notReadyServer.listen(bindAddress(), 0));

    QNetworkReply *notReadyReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(notReadyServer.serverPort(),
                                                                                             QStringLiteral("/snapshot/audio/default-sink"))));
    QSignalSpy notReadySpy(notReadyReply, &QNetworkReply::finished);
    QVERIFY(notReadySpy.wait());
    QCOMPARE(notReadyReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 503);
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(notReadyReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("not_ready"));
    notReadyReply->deleteLater();

    QNetworkReply *notReadySourceReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(notReadyServer.serverPort(),
                                                                                                   QStringLiteral("/snapshot/audio/default-source"))));
    QSignalSpy notReadySourceSpy(notReadySourceReply, &QNetworkReply::finished);
    QVERIFY(notReadySourceSpy.wait());
    QCOMPARE(notReadySourceReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 503);
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(notReadySourceReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("not_ready"));
    notReadySourceReply->deleteLater();

    plasma_bridge::state::AudioStateStore missingDefaultStore;
    plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    state.selectedSinkId.clear();
    state.sinks[0].isDefault = false;
    state.sinks[1].isDefault = false;
    missingDefaultStore.updateAudioState(state, true, QStringLiteral("initial"));

    plasma_bridge::api::SnapshotHttpServer missingDefaultServer(&missingDefaultStore, nullptr, QStringLiteral("127.0.0.1"), 18080,
                                                                18081);
    QVERIFY(missingDefaultServer.listen(bindAddress(), 0));

    QNetworkReply *missingReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(missingDefaultServer.serverPort(),
                                                                                            QStringLiteral("/snapshot/audio/default-sink"))));
    QSignalSpy missingSpy(missingReply, &QNetworkReply::finished);
    QVERIFY(missingSpy.wait());
    QCOMPARE(missingReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 404);
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(missingReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("not_found"));
    missingReply->deleteLater();

    plasma_bridge::state::AudioStateStore missingDefaultSourceStore;
    state = plasma_bridge::tests::sampleAudioState();
    state.selectedSourceId.clear();
    state.sources[0].isDefault = false;
    state.sources[1].isDefault = false;
    missingDefaultSourceStore.updateAudioState(state, true, QStringLiteral("initial"));

    plasma_bridge::api::SnapshotHttpServer missingDefaultSourceServer(&missingDefaultSourceStore,
                                                                      nullptr,
                                                                      QStringLiteral("127.0.0.1"),
                                                                      18080,
                                                                      18081);
    QVERIFY(missingDefaultSourceServer.listen(bindAddress(), 0));

    QNetworkReply *missingSourceReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(
        missingDefaultSourceServer.serverPort(),
        QStringLiteral("/snapshot/audio/default-source"))));
    QSignalSpy missingSourceSpy(missingSourceReply, &QNetworkReply::finished);
    QVERIFY(missingSourceSpy.wait());
    QCOMPARE(missingSourceReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 404);
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(missingSourceReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("not_found"));
    missingSourceReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::servesVolumeControlEndpoints()
{
    using Operation = plasma_bridge::tests::FakeAudioVolumeController::Operation;
    using Status = plasma_bridge::control::VolumeChangeStatus;

    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::tests::FakeAudioVolumeController controller;

    plasma_bridge::control::VolumeChangeResult setResult;
    setResult.status = Status::Accepted;
    setResult.sinkId = QStringLiteral("sink with space");
    setResult.requestedValue = 0.55;
    setResult.targetValue = 0.55;
    setResult.previousValue = 0.41;
    controller.setResult(Operation::Set, setResult);

    plasma_bridge::control::VolumeChangeResult incrementResult;
    incrementResult.status = Status::Accepted;
    incrementResult.sinkId = QStringLiteral("alsa_output.usb-default.analog-stereo");
    incrementResult.requestedValue = 0.1;
    incrementResult.targetValue = 0.85;
    incrementResult.previousValue = 0.75;
    controller.setResult(Operation::Increment, incrementResult);

    plasma_bridge::control::VolumeChangeResult decrementResult;
    decrementResult.status = Status::Accepted;
    decrementResult.sinkId = QStringLiteral("bluez_output.headset.1");
    decrementResult.requestedValue = 0.15;
    decrementResult.targetValue = 0.2;
    decrementResult.previousValue = 0.35;
    controller.setResult(Operation::Decrement, decrementResult);

    plasma_bridge::api::SnapshotHttpServer server(&store, &controller, QStringLiteral("127.0.0.1"), 18080, 18081);
    QVERIFY(server.listen(bindAddress(), 0));

    QNetworkAccessManager manager;

    QNetworkReply *setReply = postJson(&manager,
                                       plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                     QStringLiteral("/control/audio/sinks/sink%20with%20space/volume")),
                                       QJsonObject{{QStringLiteral("value"), 0.55}},
                                       QByteArrayLiteral("application/json; charset=utf-8"));
    QSignalSpy setSpy(setReply, &QNetworkReply::finished);
    QVERIFY(setSpy.wait());
    QCOMPARE(setReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    const QJsonObject setJson = plasma_bridge::tests::parseJsonObject(readReplyBody(setReply));
    QCOMPARE(setJson.value(QStringLiteral("status")).toString(), QStringLiteral("accepted"));
    QCOMPARE(setJson.value(QStringLiteral("sinkId")).toString(), QStringLiteral("sink with space"));
    QCOMPARE(setJson.value(QStringLiteral("requestedValue")).toDouble(), 0.55);
    QCOMPARE(setJson.value(QStringLiteral("targetValue")).toDouble(), 0.55);
    QCOMPARE(setJson.value(QStringLiteral("previousValue")).toDouble(), 0.41);
    QCOMPARE(controller.lastOperation(), Operation::Set);
    QCOMPARE(controller.lastSinkId(), QStringLiteral("sink with space"));
    QCOMPARE(controller.lastValue(), 0.55);
    QCOMPARE(controller.callCount(), 1);
    setReply->deleteLater();

    QNetworkReply *incrementReply = postJson(
        &manager,
        plasma_bridge::tests::httpUrl(server.serverPort(),
                                      QStringLiteral("/control/audio/sinks/alsa_output.usb-default.analog-stereo/volume/increment")),
        QJsonObject{{QStringLiteral("value"), 0.1}});
    QSignalSpy incrementSpy(incrementReply, &QNetworkReply::finished);
    QVERIFY(incrementSpy.wait());
    QCOMPARE(incrementReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    const QJsonObject incrementJson = plasma_bridge::tests::parseJsonObject(readReplyBody(incrementReply));
    QCOMPARE(incrementJson.value(QStringLiteral("status")).toString(), QStringLiteral("accepted"));
    QCOMPARE(incrementJson.value(QStringLiteral("sinkId")).toString(), QStringLiteral("alsa_output.usb-default.analog-stereo"));
    QCOMPARE(incrementJson.value(QStringLiteral("requestedValue")).toDouble(), 0.1);
    QCOMPARE(incrementJson.value(QStringLiteral("targetValue")).toDouble(), 0.85);
    QCOMPARE(incrementJson.value(QStringLiteral("previousValue")).toDouble(), 0.75);
    QCOMPARE(controller.lastOperation(), Operation::Increment);
    QCOMPARE(controller.lastSinkId(), QStringLiteral("alsa_output.usb-default.analog-stereo"));
    QCOMPARE(controller.lastValue(), 0.1);
    QCOMPARE(controller.callCount(), 2);
    incrementReply->deleteLater();

    QNetworkReply *decrementReply = postJson(
        &manager,
        plasma_bridge::tests::httpUrl(server.serverPort(),
                                      QStringLiteral("/control/audio/sinks/bluez_output.headset.1/volume/decrement")),
        QJsonObject{{QStringLiteral("value"), 0.15}});
    QSignalSpy decrementSpy(decrementReply, &QNetworkReply::finished);
    QVERIFY(decrementSpy.wait());
    QCOMPARE(decrementReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    const QJsonObject decrementJson = plasma_bridge::tests::parseJsonObject(readReplyBody(decrementReply));
    QCOMPARE(decrementJson.value(QStringLiteral("status")).toString(), QStringLiteral("accepted"));
    QCOMPARE(decrementJson.value(QStringLiteral("sinkId")).toString(), QStringLiteral("bluez_output.headset.1"));
    QCOMPARE(decrementJson.value(QStringLiteral("requestedValue")).toDouble(), 0.15);
    QCOMPARE(decrementJson.value(QStringLiteral("targetValue")).toDouble(), 0.2);
    QCOMPARE(decrementJson.value(QStringLiteral("previousValue")).toDouble(), 0.35);
    QCOMPARE(controller.lastOperation(), Operation::Decrement);
    QCOMPARE(controller.lastSinkId(), QStringLiteral("bluez_output.headset.1"));
    QCOMPARE(controller.lastValue(), 0.15);
    QCOMPARE(controller.callCount(), 3);
    decrementReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::mapsVolumeControlFailures()
{
    using Operation = plasma_bridge::tests::FakeAudioVolumeController::Operation;
    using Status = plasma_bridge::control::VolumeChangeStatus;

    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::tests::FakeAudioVolumeController controller;
    plasma_bridge::api::SnapshotHttpServer server(&store, &controller, QStringLiteral("127.0.0.1"), 18080, 18081);
    QVERIFY(server.listen(bindAddress(), 0));

    QNetworkAccessManager manager;

    plasma_bridge::control::VolumeChangeResult notFoundResult;
    notFoundResult.status = Status::SinkNotFound;
    notFoundResult.sinkId = QStringLiteral("missing-sink");
    notFoundResult.requestedValue = 0.25;
    controller.setResult(Operation::Set, notFoundResult);

    QNetworkReply *notFoundReply =
        postJson(&manager,
                 plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/missing-sink/volume")),
                 QJsonObject{{QStringLiteral("value"), 0.25}});
    QSignalSpy notFoundSpy(notFoundReply, &QNetworkReply::finished);
    QVERIFY(notFoundSpy.wait());
    QCOMPARE(notFoundReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 404);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(notFoundReply));
        QCOMPARE(body.value(QStringLiteral("status")).toString(), QStringLiteral("sink_not_found"));
        QCOMPARE(body.value(QStringLiteral("sinkId")).toString(), notFoundResult.sinkId);
    }
    notFoundReply->deleteLater();

    plasma_bridge::control::VolumeChangeResult notWritableResult;
    notWritableResult.status = Status::SinkNotWritable;
    notWritableResult.sinkId = QStringLiteral("bluez_output.headset.1");
    notWritableResult.requestedValue = 0.25;
    notWritableResult.previousValue = 0.35;
    controller.setResult(Operation::Increment, notWritableResult);

    QNetworkReply *notWritableReply = postJson(
        &manager,
        plasma_bridge::tests::httpUrl(server.serverPort(),
                                      QStringLiteral("/control/audio/sinks/bluez_output.headset.1/volume/increment")),
        QJsonObject{{QStringLiteral("value"), 0.25}});
    QSignalSpy notWritableSpy(notWritableReply, &QNetworkReply::finished);
    QVERIFY(notWritableSpy.wait());
    QCOMPARE(notWritableReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 409);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(notWritableReply));
        QCOMPARE(body.value(QStringLiteral("status")).toString(), QStringLiteral("sink_not_writable"));
        QCOMPARE(body.value(QStringLiteral("sinkId")).toString(), notWritableResult.sinkId);
    }
    notWritableReply->deleteLater();

    plasma_bridge::control::VolumeChangeResult notReadyResult;
    notReadyResult.status = Status::NotReady;
    notReadyResult.sinkId = QStringLiteral("alsa_output.usb-default.analog-stereo");
    notReadyResult.requestedValue = 0.25;
    controller.setResult(Operation::Decrement, notReadyResult);

    QNetworkReply *notReadyReply = postJson(
        &manager,
        plasma_bridge::tests::httpUrl(server.serverPort(),
                                      QStringLiteral("/control/audio/sinks/alsa_output.usb-default.analog-stereo/volume/decrement")),
        QJsonObject{{QStringLiteral("value"), 0.25}});
    QSignalSpy notReadySpy(notReadyReply, &QNetworkReply::finished);
    QVERIFY(notReadySpy.wait());
    QCOMPARE(notReadyReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 503);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(notReadyReply));
        QCOMPARE(body.value(QStringLiteral("status")).toString(), QStringLiteral("not_ready"));
        QCOMPARE(body.value(QStringLiteral("sinkId")).toString(), notReadyResult.sinkId);
    }
    notReadyReply->deleteLater();

    plasma_bridge::control::VolumeChangeResult invalidValueResult;
    invalidValueResult.status = Status::InvalidValue;
    invalidValueResult.sinkId = QStringLiteral("alsa_output.usb-default.analog-stereo");
    invalidValueResult.requestedValue = 0.25;
    controller.setResult(Operation::Set, invalidValueResult);

    QNetworkReply *invalidValueReply =
        postJson(&manager,
                 plasma_bridge::tests::httpUrl(server.serverPort(),
                                               QStringLiteral("/control/audio/sinks/alsa_output.usb-default.analog-stereo/volume")),
                 QJsonObject{{QStringLiteral("value"), 0.25}});
    QSignalSpy invalidValueSpy(invalidValueReply, &QNetworkReply::finished);
    QVERIFY(invalidValueSpy.wait());
    QCOMPARE(invalidValueReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 400);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(invalidValueReply));
        QCOMPARE(body.value(QStringLiteral("status")).toString(), QStringLiteral("invalid_value"));
        QCOMPARE(body.value(QStringLiteral("sinkId")).toString(), invalidValueResult.sinkId);
    }
    invalidValueReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::handlesMethodNotAllowedAndUnknownPath()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::tests::FakeAudioVolumeController controller;
    plasma_bridge::api::SnapshotHttpServer server(&store, &controller, QStringLiteral("127.0.0.1"), 18080, 18081);
    QVERIFY(server.listen(bindAddress(), 0));

    QNetworkAccessManager manager;

    QNetworkRequest snapshotRequest(plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/snapshot/audio/sinks")));
    QNetworkReply *snapshotPostReply = manager.sendCustomRequest(snapshotRequest, "POST");
    QSignalSpy snapshotPostSpy(snapshotPostReply, &QNetworkReply::finished);
    QVERIFY(snapshotPostSpy.wait());
    QCOMPARE(snapshotPostReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 405);
    QCOMPARE(snapshotPostReply->rawHeader("Allow"), QByteArrayLiteral("GET"));
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(snapshotPostReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("method_not_allowed"));
    snapshotPostReply->deleteLater();

    QNetworkRequest sourcesRequest(plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/snapshot/audio/sources")));
    QNetworkReply *sourcesPostReply = manager.sendCustomRequest(sourcesRequest, "POST");
    QSignalSpy sourcesPostSpy(sourcesPostReply, &QNetworkReply::finished);
    QVERIFY(sourcesPostSpy.wait());
    QCOMPARE(sourcesPostReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 405);
    QCOMPARE(sourcesPostReply->rawHeader("Allow"), QByteArrayLiteral("GET"));
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(sourcesPostReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("method_not_allowed"));
    sourcesPostReply->deleteLater();

    QNetworkReply *controlGetReply = manager.get(QNetworkRequest(
        plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/sink-1/volume"))));
    QSignalSpy controlGetSpy(controlGetReply, &QNetworkReply::finished);
    QVERIFY(controlGetSpy.wait());
    QCOMPARE(controlGetReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 405);
    QCOMPARE(controlGetReply->rawHeader("Allow"), QByteArrayLiteral("POST"));
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(controlGetReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("method_not_allowed"));
    controlGetReply->deleteLater();

    QNetworkReply *notFoundReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                                             QStringLiteral("/unknown"))));
    QSignalSpy notFoundSpy(notFoundReply, &QNetworkReply::finished);
    QVERIFY(notFoundSpy.wait());
    QCOMPARE(notFoundReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 404);
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(notFoundReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("not_found"));
    notFoundReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::rejectsInvalidVolumeControlRequests()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::tests::FakeAudioVolumeController controller;
    plasma_bridge::api::SnapshotHttpServer server(&store, &controller, QStringLiteral("127.0.0.1"), 18080, 18081);
    QVERIFY(server.listen(bindAddress(), 0));

    QNetworkAccessManager manager;

    QNetworkReply *wrongContentTypeReply =
        postBytes(&manager,
                  plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/sink-1/volume")),
                  QByteArrayLiteral("{\"value\":0.5}"),
                  QByteArrayLiteral("text/plain"));
    QSignalSpy wrongContentTypeSpy(wrongContentTypeReply, &QNetworkReply::finished);
    QVERIFY(wrongContentTypeSpy.wait());
    QCOMPARE(wrongContentTypeReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 415);
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(wrongContentTypeReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("unsupported_media_type"));
    QCOMPARE(controller.callCount(), 0);
    wrongContentTypeReply->deleteLater();

    QNetworkReply *malformedJsonReply =
        postBytes(&manager,
                  plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/sink-1/volume")),
                  QByteArrayLiteral("{"),
                  QByteArrayLiteral("application/json"));
    QSignalSpy malformedJsonSpy(malformedJsonReply, &QNetworkReply::finished);
    QVERIFY(malformedJsonSpy.wait());
    QCOMPARE(malformedJsonReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 400);
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(malformedJsonReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("bad_request"));
    QCOMPARE(controller.callCount(), 0);
    malformedJsonReply->deleteLater();

    QNetworkReply *missingValueReply =
        postJson(&manager,
                 plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/sink-1/volume")),
                 QJsonObject{});
    QSignalSpy missingValueSpy(missingValueReply, &QNetworkReply::finished);
    QVERIFY(missingValueSpy.wait());
    QCOMPARE(missingValueReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 400);
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(missingValueReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("bad_request"));
    QCOMPARE(controller.callCount(), 0);
    missingValueReply->deleteLater();

    QNetworkReply *emptyBodyReply =
        postBytes(&manager,
                  plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/sink-1/volume")),
                  QByteArray(),
                  QByteArrayLiteral("application/json"));
    QSignalSpy emptyBodySpy(emptyBodyReply, &QNetworkReply::finished);
    QVERIFY(emptyBodySpy.wait());
    QCOMPARE(emptyBodyReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 400);
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(emptyBodyReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("bad_request"));
    QCOMPARE(controller.callCount(), 0);
    emptyBodyReply->deleteLater();

    QNetworkReply *encodedSlashReply =
        postJson(&manager,
                 plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/sink%2Fpart/volume")),
                 QJsonObject{{QStringLiteral("value"), 0.5}});
    QSignalSpy encodedSlashSpy(encodedSlashReply, &QNetworkReply::finished);
    QVERIFY(encodedSlashSpy.wait());
    QCOMPARE(encodedSlashReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 400);
    QCOMPARE(plasma_bridge::tests::parseJsonObject(readReplyBody(encodedSlashReply)).value(QStringLiteral("code")).toString(),
             QStringLiteral("bad_request"));
    QCOMPARE(controller.callCount(), 0);
    encodedSlashReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::rejectsMalformedAndOversizedRequests()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::tests::FakeAudioVolumeController controller;
    plasma_bridge::api::SnapshotHttpServer server(&store, &controller, QStringLiteral("127.0.0.1"), 18080, 18081);
    QVERIFY(server.listen(bindAddress(), 0));

    InMemoryTcpSocket malformedSocket;
    server.processRequest(&malformedSocket, QByteArrayLiteral("GET /snapshot/audio/sinks"));
    QVERIFY(malformedSocket.writtenData().startsWith("HTTP/1.1 400 Bad Request"));
    QVERIFY(malformedSocket.writtenData().contains("\"code\":\"bad_request\""));

    InMemoryTcpSocket missingLengthSocket;
    server.processRequest(&missingLengthSocket,
                          QByteArrayLiteral("POST /control/audio/sinks/sink-1/volume HTTP/1.1\r\n"
                                            "Host: 127.0.0.1\r\n"
                                            "Content-Type: application/json\r\n"
                                            "\r\n"
                                            "{\"value\":0.5}"));
    QVERIFY(missingLengthSocket.writtenData().startsWith("HTTP/1.1 400 Bad Request"));
    QVERIFY(missingLengthSocket.writtenData().contains("\"message\":\"Content-Length is required for POST requests.\""));
    QCOMPARE(controller.callCount(), 0);

    InMemoryTcpSocket transferEncodingSocket;
    server.processRequest(&transferEncodingSocket,
                          QByteArrayLiteral("POST /control/audio/sinks/sink-1/volume HTTP/1.1\r\n"
                                            "Host: 127.0.0.1\r\n"
                                            "Content-Type: application/json\r\n"
                                            "Transfer-Encoding: chunked\r\n"
                                            "\r\n"));
    QVERIFY(transferEncodingSocket.writtenData().startsWith("HTTP/1.1 400 Bad Request"));
    QVERIFY(transferEncodingSocket.writtenData().contains("\"message\":\"Transfer-Encoding is not supported.\""));
    QCOMPARE(controller.callCount(), 0);

    QByteArray oversizedBody(17000, 'a');
    QByteArray oversizedRequest = QByteArrayLiteral("POST /control/audio/sinks/sink-1/volume HTTP/1.1\r\n"
                                                    "Host: 127.0.0.1\r\n"
                                                    "Content-Type: application/json\r\n"
                                                    "Content-Length: ");
    oversizedRequest += QByteArray::number(oversizedBody.size());
    oversizedRequest += QByteArrayLiteral("\r\n\r\n");
    oversizedRequest += oversizedBody;

    InMemoryTcpSocket oversizedSocket(oversizedRequest);
    server.handleReadyRead(&oversizedSocket);
    QVERIFY(oversizedSocket.writtenData().startsWith("HTTP/1.1 400 Bad Request"));
    QVERIFY(oversizedSocket.writtenData().contains("\"message\":\"Request is too large.\""));
    QCOMPARE(controller.callCount(), 0);
}

void SnapshotHttpServerFeatureTest::servesDocsAndRewritesSpecHosts()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::api::SnapshotHttpServer server(&store, nullptr, QStringLiteral("localhost"), 19080, 19081);
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
    QVERIFY(openApiBody.contains(QStringLiteral("/snapshot/audio/sources")));
    QVERIFY(openApiBody.contains(QStringLiteral("/snapshot/audio/default-source")));
    QVERIFY(openApiBody.contains(QStringLiteral("/control/audio/sinks/{sinkId}/volume")));
    QVERIFY(openApiBody.contains(QStringLiteral("/control/audio/sinks/{sinkId}/volume/increment")));
    QVERIFY(openApiBody.contains(QStringLiteral("/control/audio/sinks/{sinkId}/volume/decrement")));
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

QNetworkReply *SnapshotHttpServerFeatureTest::postBytes(QNetworkAccessManager *manager,
                                                        const QUrl &url,
                                                        const QByteArray &body,
                                                        const QByteArray &contentType)
{
    if (manager == nullptr) {
        return nullptr;
    }

    QNetworkRequest request(url);
    request.setRawHeader("Content-Type", contentType);
    return manager->post(request, body);
}

QNetworkReply *SnapshotHttpServerFeatureTest::postJson(QNetworkAccessManager *manager,
                                                       const QUrl &url,
                                                       const QJsonObject &body,
                                                       const QByteArray &contentType)
{
    return postBytes(manager, url, QJsonDocument(body).toJson(QJsonDocument::Compact), contentType);
}

QTEST_GUILESS_MAIN(SnapshotHttpServerFeatureTest)

#include "test_snapshot_http_server.moc"
