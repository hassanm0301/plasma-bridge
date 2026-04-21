#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QtTest>

#include <cstring>

#include "plasma_bridge_build_config.h"

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
    void servesDeviceControlEndpoints();
    void mapsDeviceControlFailures();
    void handlesMethodNotAllowedAndUnknownPath();
    void rejectsInvalidVolumeControlRequests();
    void rejectsInvalidDeviceControlRequests();
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

namespace
{

QJsonObject payloadObject(const QJsonObject &envelope)
{
    return envelope.value(QStringLiteral("payload")).toObject();
}

QJsonObject errorObject(const QJsonObject &envelope)
{
    return envelope.value(QStringLiteral("error")).toObject();
}

} // namespace

void SnapshotHttpServerFeatureTest::initTestCase()
{
    plasma_bridge::tests::ensureDocsResourcesInitialized();
}

void SnapshotHttpServerFeatureTest::servesSnapshotAndDefaultSinkEndpoints()
{
    plasma_bridge::state::AudioStateStore store;
    const plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    store.updateAudioState(state, true, QStringLiteral("initial"));

    plasma_bridge::api::SnapshotHttpServer server(&store,
                                                  nullptr,
                                                  nullptr,
                                                  QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST),
                                                  18080,
                                                  18081);
    QVERIFY(server.listen(bindAddress(), 0));

    QNetworkAccessManager manager;

    QNetworkReply *sinksReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                                          QStringLiteral("/snapshot/audio/sinks"))));
    QSignalSpy sinksSpy(sinksReply, &QNetworkReply::finished);
    QVERIFY(sinksSpy.wait());
    QCOMPARE(sinksReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    const QJsonObject sinksEnvelope = plasma_bridge::tests::parseJsonObject(readReplyBody(sinksReply));
    QVERIFY(sinksEnvelope.value(QStringLiteral("error")).isNull());
    const QJsonObject sinksJson = payloadObject(sinksEnvelope);
    QCOMPARE(sinksJson.value(QStringLiteral("selectedSinkId")).toString(), state.selectedSinkId);
    QCOMPARE(sinksJson.value(QStringLiteral("sinks")).toArray().size(), state.sinks.size());
    QVERIFY(!sinksJson.contains(QStringLiteral("sources")));
    sinksReply->deleteLater();

    QNetworkReply *defaultReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                                            QStringLiteral("/snapshot/audio/default-sink"))));
    QSignalSpy defaultSpy(defaultReply, &QNetworkReply::finished);
    QVERIFY(defaultSpy.wait());
    QCOMPARE(defaultReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    const QJsonObject defaultEnvelope = plasma_bridge::tests::parseJsonObject(readReplyBody(defaultReply));
    QVERIFY(defaultEnvelope.value(QStringLiteral("error")).isNull());
    const QJsonObject defaultJson = payloadObject(defaultEnvelope);
    QCOMPARE(defaultJson.value(QStringLiteral("id")).toString(), state.selectedSinkId);
    defaultReply->deleteLater();

    QNetworkReply *sourcesReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                                            QStringLiteral("/snapshot/audio/sources"))));
    QSignalSpy sourcesSpy(sourcesReply, &QNetworkReply::finished);
    QVERIFY(sourcesSpy.wait());
    QCOMPARE(sourcesReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    const QJsonObject sourcesEnvelope = plasma_bridge::tests::parseJsonObject(readReplyBody(sourcesReply));
    QVERIFY(sourcesEnvelope.value(QStringLiteral("error")).isNull());
    const QJsonObject sourcesJson = payloadObject(sourcesEnvelope);
    QCOMPARE(sourcesJson.value(QStringLiteral("selectedSourceId")).toString(), state.selectedSourceId);
    QCOMPARE(sourcesJson.value(QStringLiteral("sources")).toArray().size(), state.sources.size());
    QVERIFY(!sourcesJson.contains(QStringLiteral("sinks")));
    sourcesReply->deleteLater();

    QNetworkReply *defaultSourceReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                                                  QStringLiteral("/snapshot/audio/default-source"))));
    QSignalSpy defaultSourceSpy(defaultSourceReply, &QNetworkReply::finished);
    QVERIFY(defaultSourceSpy.wait());
    QCOMPARE(defaultSourceReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    const QJsonObject defaultSourceEnvelope = plasma_bridge::tests::parseJsonObject(readReplyBody(defaultSourceReply));
    QVERIFY(defaultSourceEnvelope.value(QStringLiteral("error")).isNull());
    const QJsonObject defaultSourceJson = payloadObject(defaultSourceEnvelope);
    QCOMPARE(defaultSourceJson.value(QStringLiteral("id")).toString(), state.selectedSourceId);
    defaultSourceReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::reportsNotReadyAndMissingDefaultSink()
{
    QNetworkAccessManager manager;

    plasma_bridge::state::AudioStateStore notReadyStore;
    plasma_bridge::api::SnapshotHttpServer notReadyServer(&notReadyStore,
                                                          nullptr,
                                                          nullptr,
                                                          QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST),
                                                          18080,
                                                          18081);
    QVERIFY(notReadyServer.listen(bindAddress(), 0));

    QNetworkReply *notReadyReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(notReadyServer.serverPort(),
                                                                                             QStringLiteral("/snapshot/audio/default-sink"))));
    QSignalSpy notReadySpy(notReadyReply, &QNetworkReply::finished);
    QVERIFY(notReadySpy.wait());
    QCOMPARE(notReadyReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 503);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(notReadyReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("not_ready"));
    }
    notReadyReply->deleteLater();

    QNetworkReply *notReadySourceReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(notReadyServer.serverPort(),
                                                                                                   QStringLiteral("/snapshot/audio/default-source"))));
    QSignalSpy notReadySourceSpy(notReadySourceReply, &QNetworkReply::finished);
    QVERIFY(notReadySourceSpy.wait());
    QCOMPARE(notReadySourceReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 503);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(notReadySourceReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("not_ready"));
    }
    notReadySourceReply->deleteLater();

    plasma_bridge::state::AudioStateStore missingDefaultStore;
    plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    state.selectedSinkId.clear();
    state.sinks[0].isDefault = false;
    state.sinks[1].isDefault = false;
    missingDefaultStore.updateAudioState(state, true, QStringLiteral("initial"));

    plasma_bridge::api::SnapshotHttpServer missingDefaultServer(&missingDefaultStore,
                                                                nullptr,
                                                                nullptr,
                                                                QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST),
                                                                18080,
                                                                18081);
    QVERIFY(missingDefaultServer.listen(bindAddress(), 0));

    QNetworkReply *missingReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(missingDefaultServer.serverPort(),
                                                                                            QStringLiteral("/snapshot/audio/default-sink"))));
    QSignalSpy missingSpy(missingReply, &QNetworkReply::finished);
    QVERIFY(missingSpy.wait());
    QCOMPARE(missingReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 404);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(missingReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("not_found"));
    }
    missingReply->deleteLater();

    plasma_bridge::state::AudioStateStore missingDefaultSourceStore;
    state = plasma_bridge::tests::sampleAudioState();
    state.selectedSourceId.clear();
    state.sources[0].isDefault = false;
    state.sources[1].isDefault = false;
    missingDefaultSourceStore.updateAudioState(state, true, QStringLiteral("initial"));

    plasma_bridge::api::SnapshotHttpServer missingDefaultSourceServer(&missingDefaultSourceStore,
                                                                      nullptr,
                                                                      nullptr,
                                                                      QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST),
                                                                      18080,
                                                                      18081);
    QVERIFY(missingDefaultSourceServer.listen(bindAddress(), 0));

    QNetworkReply *missingSourceReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(
        missingDefaultSourceServer.serverPort(),
        QStringLiteral("/snapshot/audio/default-source"))));
    QSignalSpy missingSourceSpy(missingSourceReply, &QNetworkReply::finished);
    QVERIFY(missingSourceSpy.wait());
    QCOMPARE(missingSourceReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 404);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(missingSourceReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("not_found"));
    }
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

    plasma_bridge::api::SnapshotHttpServer server(&store,
                                                  &controller,
                                                  nullptr,
                                                  QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST),
                                                  18080,
                                                  18081);
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
    const QJsonObject setEnvelope = plasma_bridge::tests::parseJsonObject(readReplyBody(setReply));
    QVERIFY(setEnvelope.value(QStringLiteral("error")).isNull());
    const QJsonObject setJson = payloadObject(setEnvelope);
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
    const QJsonObject incrementEnvelope = plasma_bridge::tests::parseJsonObject(readReplyBody(incrementReply));
    QVERIFY(incrementEnvelope.value(QStringLiteral("error")).isNull());
    const QJsonObject incrementJson = payloadObject(incrementEnvelope);
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
    const QJsonObject decrementEnvelope = plasma_bridge::tests::parseJsonObject(readReplyBody(decrementReply));
    QVERIFY(decrementEnvelope.value(QStringLiteral("error")).isNull());
    const QJsonObject decrementJson = payloadObject(decrementEnvelope);
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
    plasma_bridge::api::SnapshotHttpServer server(&store,
                                                  &controller,
                                                  nullptr,
                                                  QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST),
                                                  18080,
                                                  18081);
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
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        const QJsonObject error = errorObject(body);
        QCOMPARE(error.value(QStringLiteral("code")).toString(), QStringLiteral("sink_not_found"));
        QCOMPARE(error.value(QStringLiteral("details")).toObject().value(QStringLiteral("sinkId")).toString(), notFoundResult.sinkId);
        QCOMPARE(error.value(QStringLiteral("details")).toObject().value(QStringLiteral("requestedValue")).toDouble(), 0.25);
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
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        const QJsonObject error = errorObject(body);
        QCOMPARE(error.value(QStringLiteral("code")).toString(), QStringLiteral("sink_not_writable"));
        QCOMPARE(error.value(QStringLiteral("details")).toObject().value(QStringLiteral("sinkId")).toString(), notWritableResult.sinkId);
        QCOMPARE(error.value(QStringLiteral("details")).toObject().value(QStringLiteral("previousValue")).toDouble(), 0.35);
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
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        const QJsonObject error = errorObject(body);
        QCOMPARE(error.value(QStringLiteral("code")).toString(), QStringLiteral("not_ready"));
        QCOMPARE(error.value(QStringLiteral("details")).toObject().value(QStringLiteral("sinkId")).toString(), notReadyResult.sinkId);
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
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        const QJsonObject error = errorObject(body);
        QCOMPARE(error.value(QStringLiteral("code")).toString(), QStringLiteral("invalid_value"));
        QCOMPARE(error.value(QStringLiteral("details")).toObject().value(QStringLiteral("sinkId")).toString(), invalidValueResult.sinkId);
    }
    invalidValueReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::servesDeviceControlEndpoints()
{
    using Operation = plasma_bridge::tests::FakeAudioDeviceController::Operation;
    using Status = plasma_bridge::control::AudioDeviceChangeStatus;

    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::tests::FakeAudioDeviceController controller;

    plasma_bridge::control::DefaultDeviceChangeResult defaultSinkResult;
    defaultSinkResult.status = Status::Accepted;
    defaultSinkResult.deviceId = QStringLiteral("sink with space");
    controller.setDefaultResult(Operation::SetDefaultSink, defaultSinkResult);

    plasma_bridge::control::DefaultDeviceChangeResult defaultSourceResult;
    defaultSourceResult.status = Status::Accepted;
    defaultSourceResult.deviceId = QStringLiteral("source with space");
    controller.setDefaultResult(Operation::SetDefaultSource, defaultSourceResult);

    plasma_bridge::control::MuteChangeResult sinkMuteResult;
    sinkMuteResult.status = Status::Accepted;
    sinkMuteResult.deviceId = QStringLiteral("bluez_output.headset.1");
    sinkMuteResult.requestedMuted = false;
    sinkMuteResult.targetMuted = false;
    sinkMuteResult.previousMuted = true;
    controller.setMuteResult(Operation::SetSinkMute, sinkMuteResult);

    plasma_bridge::control::MuteChangeResult sourceMuteResult;
    sourceMuteResult.status = Status::Accepted;
    sourceMuteResult.deviceId = QStringLiteral("alsa_input.usb-default.analog-stereo");
    sourceMuteResult.requestedMuted = true;
    sourceMuteResult.targetMuted = true;
    sourceMuteResult.previousMuted = false;
    controller.setMuteResult(Operation::SetSourceMute, sourceMuteResult);

    plasma_bridge::api::SnapshotHttpServer server(&store,
                                                  nullptr,
                                                  &controller,
                                                  QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST),
                                                  18080,
                                                  18081);
    QVERIFY(server.listen(bindAddress(), 0));

    QNetworkAccessManager manager;

    QNetworkRequest setDefaultSinkRequest(
        plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/sink%20with%20space/default")));
    QNetworkReply *setDefaultSinkReply = manager.post(setDefaultSinkRequest, QByteArray());
    QSignalSpy setDefaultSinkSpy(setDefaultSinkReply, &QNetworkReply::finished);
    QVERIFY(setDefaultSinkSpy.wait());
    QCOMPARE(setDefaultSinkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    {
        const QJsonObject envelope = plasma_bridge::tests::parseJsonObject(readReplyBody(setDefaultSinkReply));
        QVERIFY(envelope.value(QStringLiteral("error")).isNull());
        const QJsonObject body = payloadObject(envelope);
        QCOMPARE(body.value(QStringLiteral("deviceId")).toString(), QStringLiteral("sink with space"));
        QCOMPARE(controller.lastOperation(), Operation::SetDefaultSink);
        QCOMPARE(controller.lastDeviceId(), QStringLiteral("sink with space"));
        QCOMPARE(controller.callCount(), 1);
    }
    setDefaultSinkReply->deleteLater();

    QNetworkRequest setDefaultSourceRequest(
        plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sources/source%20with%20space/default")));
    QNetworkReply *setDefaultSourceReply = manager.post(setDefaultSourceRequest, QByteArray());
    QSignalSpy setDefaultSourceSpy(setDefaultSourceReply, &QNetworkReply::finished);
    QVERIFY(setDefaultSourceSpy.wait());
    QCOMPARE(setDefaultSourceReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    {
        const QJsonObject envelope = plasma_bridge::tests::parseJsonObject(readReplyBody(setDefaultSourceReply));
        QVERIFY(envelope.value(QStringLiteral("error")).isNull());
        const QJsonObject body = payloadObject(envelope);
        QCOMPARE(body.value(QStringLiteral("deviceId")).toString(), QStringLiteral("source with space"));
        QCOMPARE(controller.lastOperation(), Operation::SetDefaultSource);
        QCOMPARE(controller.lastDeviceId(), QStringLiteral("source with space"));
        QCOMPARE(controller.callCount(), 2);
    }
    setDefaultSourceReply->deleteLater();

    QNetworkReply *sinkMuteReply = postJson(
        &manager,
        plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/bluez_output.headset.1/mute")),
        QJsonObject{{QStringLiteral("muted"), false}});
    QSignalSpy sinkMuteSpy(sinkMuteReply, &QNetworkReply::finished);
    QVERIFY(sinkMuteSpy.wait());
    QCOMPARE(sinkMuteReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    {
        const QJsonObject envelope = plasma_bridge::tests::parseJsonObject(readReplyBody(sinkMuteReply));
        QVERIFY(envelope.value(QStringLiteral("error")).isNull());
        const QJsonObject body = payloadObject(envelope);
        QCOMPARE(body.value(QStringLiteral("deviceId")).toString(), QStringLiteral("bluez_output.headset.1"));
        QCOMPARE(body.value(QStringLiteral("requestedMuted")).toBool(), false);
        QCOMPARE(body.value(QStringLiteral("targetMuted")).toBool(), false);
        QCOMPARE(body.value(QStringLiteral("previousMuted")).toBool(), true);
        QCOMPARE(controller.lastOperation(), Operation::SetSinkMute);
        QCOMPARE(controller.lastDeviceId(), QStringLiteral("bluez_output.headset.1"));
        QVERIFY(controller.lastMuted().has_value());
        QCOMPARE(*controller.lastMuted(), false);
        QCOMPARE(controller.callCount(), 3);
    }
    sinkMuteReply->deleteLater();

    QNetworkReply *sourceMuteReply = postJson(
        &manager,
        plasma_bridge::tests::httpUrl(server.serverPort(),
                                      QStringLiteral("/control/audio/sources/alsa_input.usb-default.analog-stereo/mute")),
        QJsonObject{{QStringLiteral("muted"), true}});
    QSignalSpy sourceMuteSpy(sourceMuteReply, &QNetworkReply::finished);
    QVERIFY(sourceMuteSpy.wait());
    QCOMPARE(sourceMuteReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    {
        const QJsonObject envelope = plasma_bridge::tests::parseJsonObject(readReplyBody(sourceMuteReply));
        QVERIFY(envelope.value(QStringLiteral("error")).isNull());
        const QJsonObject body = payloadObject(envelope);
        QCOMPARE(body.value(QStringLiteral("deviceId")).toString(), QStringLiteral("alsa_input.usb-default.analog-stereo"));
        QCOMPARE(body.value(QStringLiteral("requestedMuted")).toBool(), true);
        QCOMPARE(body.value(QStringLiteral("targetMuted")).toBool(), true);
        QCOMPARE(body.value(QStringLiteral("previousMuted")).toBool(), false);
        QCOMPARE(controller.lastOperation(), Operation::SetSourceMute);
        QCOMPARE(controller.lastDeviceId(), QStringLiteral("alsa_input.usb-default.analog-stereo"));
        QVERIFY(controller.lastMuted().has_value());
        QCOMPARE(*controller.lastMuted(), true);
        QCOMPARE(controller.callCount(), 4);
    }
    sourceMuteReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::mapsDeviceControlFailures()
{
    using Operation = plasma_bridge::tests::FakeAudioDeviceController::Operation;
    using Status = plasma_bridge::control::AudioDeviceChangeStatus;

    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::tests::FakeAudioDeviceController controller;
    plasma_bridge::api::SnapshotHttpServer server(&store,
                                                  nullptr,
                                                  &controller,
                                                  QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST),
                                                  18080,
                                                  18081);
    QVERIFY(server.listen(bindAddress(), 0));

    QNetworkAccessManager manager;

    plasma_bridge::control::DefaultDeviceChangeResult sinkNotFoundResult;
    sinkNotFoundResult.status = Status::SinkNotFound;
    sinkNotFoundResult.deviceId = QStringLiteral("missing-sink");
    controller.setDefaultResult(Operation::SetDefaultSink, sinkNotFoundResult);

    QNetworkRequest sinkNotFoundRequest(
        plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/missing-sink/default")));
    QNetworkReply *sinkNotFoundReply = manager.post(sinkNotFoundRequest, QByteArray());
    QSignalSpy sinkNotFoundSpy(sinkNotFoundReply, &QNetworkReply::finished);
    QVERIFY(sinkNotFoundSpy.wait());
    QCOMPARE(sinkNotFoundReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 404);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(sinkNotFoundReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        const QJsonObject error = errorObject(body);
        QCOMPARE(error.value(QStringLiteral("code")).toString(), QStringLiteral("sink_not_found"));
        QCOMPARE(error.value(QStringLiteral("details")).toObject().value(QStringLiteral("deviceId")).toString(),
                 sinkNotFoundResult.deviceId);
    }
    sinkNotFoundReply->deleteLater();

    plasma_bridge::control::DefaultDeviceChangeResult sourceNotFoundResult;
    sourceNotFoundResult.status = Status::SourceNotFound;
    sourceNotFoundResult.deviceId = QStringLiteral("missing-source");
    controller.setDefaultResult(Operation::SetDefaultSource, sourceNotFoundResult);

    QNetworkRequest sourceNotFoundRequest(
        plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sources/missing-source/default")));
    QNetworkReply *sourceNotFoundReply = manager.post(sourceNotFoundRequest, QByteArray());
    QSignalSpy sourceNotFoundSpy(sourceNotFoundReply, &QNetworkReply::finished);
    QVERIFY(sourceNotFoundSpy.wait());
    QCOMPARE(sourceNotFoundReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 404);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(sourceNotFoundReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        const QJsonObject error = errorObject(body);
        QCOMPARE(error.value(QStringLiteral("code")).toString(), QStringLiteral("source_not_found"));
        QCOMPARE(error.value(QStringLiteral("details")).toObject().value(QStringLiteral("deviceId")).toString(),
                 sourceNotFoundResult.deviceId);
    }
    sourceNotFoundReply->deleteLater();

    plasma_bridge::control::DefaultDeviceChangeResult defaultNotReadyResult;
    defaultNotReadyResult.status = Status::NotReady;
    defaultNotReadyResult.deviceId = QStringLiteral("source-1");
    controller.setDefaultResult(Operation::SetDefaultSource, defaultNotReadyResult);

    QNetworkRequest defaultNotReadyRequest(
        plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sources/source-1/default")));
    QNetworkReply *defaultNotReadyReply = manager.post(defaultNotReadyRequest, QByteArray());
    QSignalSpy defaultNotReadySpy(defaultNotReadyReply, &QNetworkReply::finished);
    QVERIFY(defaultNotReadySpy.wait());
    QCOMPARE(defaultNotReadyReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 503);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(defaultNotReadyReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        const QJsonObject error = errorObject(body);
        QCOMPARE(error.value(QStringLiteral("code")).toString(), QStringLiteral("not_ready"));
        QCOMPARE(error.value(QStringLiteral("details")).toObject().value(QStringLiteral("deviceId")).toString(),
                 defaultNotReadyResult.deviceId);
    }
    defaultNotReadyReply->deleteLater();

    plasma_bridge::control::MuteChangeResult sinkNotWritableResult;
    sinkNotWritableResult.status = Status::SinkNotWritable;
    sinkNotWritableResult.deviceId = QStringLiteral("bluez_output.headset.1");
    sinkNotWritableResult.requestedMuted = true;
    sinkNotWritableResult.targetMuted = false;
    sinkNotWritableResult.previousMuted = false;
    controller.setMuteResult(Operation::SetSinkMute, sinkNotWritableResult);

    QNetworkReply *sinkNotWritableReply = postJson(
        &manager,
        plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/bluez_output.headset.1/mute")),
        QJsonObject{{QStringLiteral("muted"), true}});
    QSignalSpy sinkNotWritableSpy(sinkNotWritableReply, &QNetworkReply::finished);
    QVERIFY(sinkNotWritableSpy.wait());
    QCOMPARE(sinkNotWritableReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 409);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(sinkNotWritableReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        const QJsonObject error = errorObject(body);
        QCOMPARE(error.value(QStringLiteral("code")).toString(), QStringLiteral("sink_not_writable"));
        const QJsonObject details = error.value(QStringLiteral("details")).toObject();
        QCOMPARE(details.value(QStringLiteral("deviceId")).toString(), sinkNotWritableResult.deviceId);
        QCOMPARE(details.value(QStringLiteral("requestedMuted")).toBool(), true);
        QCOMPARE(details.value(QStringLiteral("targetMuted")).toBool(), false);
    }
    sinkNotWritableReply->deleteLater();

    plasma_bridge::control::MuteChangeResult sourceNotWritableResult;
    sourceNotWritableResult.status = Status::SourceNotWritable;
    sourceNotWritableResult.deviceId = QStringLiteral("alsa_input.usb-default.analog-stereo");
    sourceNotWritableResult.requestedMuted = false;
    sourceNotWritableResult.targetMuted = true;
    sourceNotWritableResult.previousMuted = true;
    controller.setMuteResult(Operation::SetSourceMute, sourceNotWritableResult);

    QNetworkReply *sourceNotWritableReply = postJson(
        &manager,
        plasma_bridge::tests::httpUrl(server.serverPort(),
                                      QStringLiteral("/control/audio/sources/alsa_input.usb-default.analog-stereo/mute")),
        QJsonObject{{QStringLiteral("muted"), false}});
    QSignalSpy sourceNotWritableSpy(sourceNotWritableReply, &QNetworkReply::finished);
    QVERIFY(sourceNotWritableSpy.wait());
    QCOMPARE(sourceNotWritableReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 409);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(sourceNotWritableReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        const QJsonObject error = errorObject(body);
        QCOMPARE(error.value(QStringLiteral("code")).toString(), QStringLiteral("source_not_writable"));
        const QJsonObject details = error.value(QStringLiteral("details")).toObject();
        QCOMPARE(details.value(QStringLiteral("deviceId")).toString(), sourceNotWritableResult.deviceId);
        QCOMPARE(details.value(QStringLiteral("requestedMuted")).toBool(), false);
        QCOMPARE(details.value(QStringLiteral("targetMuted")).toBool(), true);
    }
    sourceNotWritableReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::handlesMethodNotAllowedAndUnknownPath()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::tests::FakeAudioVolumeController controller;
    plasma_bridge::tests::FakeAudioDeviceController deviceController;
    plasma_bridge::api::SnapshotHttpServer server(&store,
                                                  &controller,
                                                  &deviceController,
                                                  QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST),
                                                  18080,
                                                  18081);
    QVERIFY(server.listen(bindAddress(), 0));

    QNetworkAccessManager manager;

    QNetworkRequest snapshotRequest(plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/snapshot/audio/sinks")));
    QNetworkReply *snapshotPostReply = manager.sendCustomRequest(snapshotRequest, "POST");
    QSignalSpy snapshotPostSpy(snapshotPostReply, &QNetworkReply::finished);
    QVERIFY(snapshotPostSpy.wait());
    QCOMPARE(snapshotPostReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 405);
    QCOMPARE(snapshotPostReply->rawHeader("Allow"), QByteArrayLiteral("GET"));
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(snapshotPostReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("method_not_allowed"));
    }
    snapshotPostReply->deleteLater();

    QNetworkRequest sourcesRequest(plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/snapshot/audio/sources")));
    QNetworkReply *sourcesPostReply = manager.sendCustomRequest(sourcesRequest, "POST");
    QSignalSpy sourcesPostSpy(sourcesPostReply, &QNetworkReply::finished);
    QVERIFY(sourcesPostSpy.wait());
    QCOMPARE(sourcesPostReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 405);
    QCOMPARE(sourcesPostReply->rawHeader("Allow"), QByteArrayLiteral("GET"));
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(sourcesPostReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("method_not_allowed"));
    }
    sourcesPostReply->deleteLater();

    QNetworkReply *controlGetReply = manager.get(QNetworkRequest(
        plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/sink-1/volume"))));
    QSignalSpy controlGetSpy(controlGetReply, &QNetworkReply::finished);
    QVERIFY(controlGetSpy.wait());
    QCOMPARE(controlGetReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 405);
    QCOMPARE(controlGetReply->rawHeader("Allow"), QByteArrayLiteral("POST"));
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(controlGetReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("method_not_allowed"));
    }
    controlGetReply->deleteLater();

    QNetworkReply *defaultGetReply = manager.get(QNetworkRequest(
        plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/sink-1/default"))));
    QSignalSpy defaultGetSpy(defaultGetReply, &QNetworkReply::finished);
    QVERIFY(defaultGetSpy.wait());
    QCOMPARE(defaultGetReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 405);
    QCOMPARE(defaultGetReply->rawHeader("Allow"), QByteArrayLiteral("POST"));
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(defaultGetReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("method_not_allowed"));
    }
    defaultGetReply->deleteLater();

    QNetworkReply *muteGetReply = manager.get(QNetworkRequest(
        plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sources/source-1/mute"))));
    QSignalSpy muteGetSpy(muteGetReply, &QNetworkReply::finished);
    QVERIFY(muteGetSpy.wait());
    QCOMPARE(muteGetReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 405);
    QCOMPARE(muteGetReply->rawHeader("Allow"), QByteArrayLiteral("POST"));
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(muteGetReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("method_not_allowed"));
    }
    muteGetReply->deleteLater();

    QNetworkReply *notFoundReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                                             QStringLiteral("/unknown"))));
    QSignalSpy notFoundSpy(notFoundReply, &QNetworkReply::finished);
    QVERIFY(notFoundSpy.wait());
    QCOMPARE(notFoundReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 404);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(notFoundReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("not_found"));
    }
    notFoundReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::rejectsInvalidVolumeControlRequests()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::tests::FakeAudioVolumeController controller;
    plasma_bridge::api::SnapshotHttpServer server(&store,
                                                  &controller,
                                                  nullptr,
                                                  QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST),
                                                  18080,
                                                  18081);
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
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(wrongContentTypeReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("unsupported_media_type"));
    }
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
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(malformedJsonReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("bad_request"));
    }
    QCOMPARE(controller.callCount(), 0);
    malformedJsonReply->deleteLater();

    QNetworkReply *missingValueReply =
        postJson(&manager,
                 plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/sink-1/volume")),
                 QJsonObject{});
    QSignalSpy missingValueSpy(missingValueReply, &QNetworkReply::finished);
    QVERIFY(missingValueSpy.wait());
    QCOMPARE(missingValueReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 400);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(missingValueReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("bad_request"));
    }
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
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(emptyBodyReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("bad_request"));
    }
    QCOMPARE(controller.callCount(), 0);
    emptyBodyReply->deleteLater();

    QNetworkReply *encodedSlashReply =
        postJson(&manager,
                 plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/sink%2Fpart/volume")),
                 QJsonObject{{QStringLiteral("value"), 0.5}});
    QSignalSpy encodedSlashSpy(encodedSlashReply, &QNetworkReply::finished);
    QVERIFY(encodedSlashSpy.wait());
    QCOMPARE(encodedSlashReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 400);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(encodedSlashReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("bad_request"));
    }
    QCOMPARE(controller.callCount(), 0);
    encodedSlashReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::rejectsInvalidDeviceControlRequests()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::tests::FakeAudioDeviceController controller;
    plasma_bridge::api::SnapshotHttpServer server(&store,
                                                  nullptr,
                                                  &controller,
                                                  QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST),
                                                  18080,
                                                  18081);
    QVERIFY(server.listen(bindAddress(), 0));

    QNetworkAccessManager manager;

    QNetworkReply *wrongContentTypeReply =
        postBytes(&manager,
                  plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/sink-1/mute")),
                  QByteArrayLiteral("{\"muted\":true}"),
                  QByteArrayLiteral("text/plain"));
    QSignalSpy wrongContentTypeSpy(wrongContentTypeReply, &QNetworkReply::finished);
    QVERIFY(wrongContentTypeSpy.wait());
    QCOMPARE(wrongContentTypeReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 415);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(wrongContentTypeReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("unsupported_media_type"));
    }
    QCOMPARE(controller.callCount(), 0);
    wrongContentTypeReply->deleteLater();

    QNetworkReply *malformedJsonReply =
        postBytes(&manager,
                  plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sources/source-1/mute")),
                  QByteArrayLiteral("{"),
                  QByteArrayLiteral("application/json"));
    QSignalSpy malformedJsonSpy(malformedJsonReply, &QNetworkReply::finished);
    QVERIFY(malformedJsonSpy.wait());
    QCOMPARE(malformedJsonReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 400);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(malformedJsonReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("bad_request"));
    }
    QCOMPARE(controller.callCount(), 0);
    malformedJsonReply->deleteLater();

    QNetworkReply *missingMutedReply =
        postJson(&manager,
                 plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sources/source-1/mute")),
                 QJsonObject{});
    QSignalSpy missingMutedSpy(missingMutedReply, &QNetworkReply::finished);
    QVERIFY(missingMutedSpy.wait());
    QCOMPARE(missingMutedReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 400);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(missingMutedReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("bad_request"));
    }
    QCOMPARE(controller.callCount(), 0);
    missingMutedReply->deleteLater();

    QNetworkRequest defaultRequest(
        plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sinks/sink-1/default")));
    defaultRequest.setRawHeader("Content-Type", QByteArrayLiteral("application/json"));
    QNetworkReply *unexpectedBodyReply = manager.post(defaultRequest, QByteArrayLiteral("{}"));
    QSignalSpy unexpectedBodySpy(unexpectedBodyReply, &QNetworkReply::finished);
    QVERIFY(unexpectedBodySpy.wait());
    QCOMPARE(unexpectedBodyReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 400);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(unexpectedBodyReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("bad_request"));
    }
    QCOMPARE(controller.callCount(), 0);
    unexpectedBodyReply->deleteLater();

    QNetworkReply *encodedSlashReply =
        postJson(&manager,
                 plasma_bridge::tests::httpUrl(server.serverPort(), QStringLiteral("/control/audio/sources/source%2Fpart/mute")),
                 QJsonObject{{QStringLiteral("muted"), true}});
    QSignalSpy encodedSlashSpy(encodedSlashReply, &QNetworkReply::finished);
    QVERIFY(encodedSlashSpy.wait());
    QCOMPARE(encodedSlashReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 400);
    {
        const QJsonObject body = plasma_bridge::tests::parseJsonObject(readReplyBody(encodedSlashReply));
        QVERIFY(body.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(body).value(QStringLiteral("code")).toString(), QStringLiteral("bad_request"));
    }
    QCOMPARE(controller.callCount(), 0);
    encodedSlashReply->deleteLater();
}

void SnapshotHttpServerFeatureTest::rejectsMalformedAndOversizedRequests()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::tests::FakeAudioVolumeController controller;
    plasma_bridge::api::SnapshotHttpServer server(&store,
                                                  &controller,
                                                  nullptr,
                                                  QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST),
                                                  18080,
                                                  18081);
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

    plasma_bridge::api::SnapshotHttpServer server(&store, nullptr, nullptr, QStringLiteral("localhost"), 19080, 19081);
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
    QVERIFY(openApiBody.contains(QStringLiteral("/control/audio/sinks/{sinkId}/default")));
    QVERIFY(openApiBody.contains(QStringLiteral("/control/audio/sources/{sourceId}/default")));
    QVERIFY(openApiBody.contains(QStringLiteral("/control/audio/sinks/{sinkId}/mute")));
    QVERIFY(openApiBody.contains(QStringLiteral("/control/audio/sources/{sourceId}/mute")));
    QVERIFY(openApiBody.contains(QStringLiteral("payload:")));
    QVERIFY(openApiBody.contains(QStringLiteral("error:")));
    openApiReply->deleteLater();

    QNetworkReply *asyncApiReply = manager.get(QNetworkRequest(plasma_bridge::tests::httpUrl(server.serverPort(),
                                                                                             QStringLiteral("/docs/asyncapi.yaml"))));
    QSignalSpy asyncApiSpy(asyncApiReply, &QNetworkReply::finished);
    QVERIFY(asyncApiSpy.wait());
    QCOMPARE(asyncApiReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    const QString asyncApiBody = QString::fromUtf8(readReplyBody(asyncApiReply));
    QVERIFY(asyncApiBody.contains(QStringLiteral("localhost:19081")));
    QVERIFY(asyncApiBody.contains(QStringLiteral("protocolVersion: 2")));
    QVERIFY(asyncApiBody.contains(QStringLiteral("payload:")));
    QVERIFY(asyncApiBody.contains(QStringLiteral("error:")));
    asyncApiReply->deleteLater();
}

QHostAddress SnapshotHttpServerFeatureTest::bindAddress()
{
    return QHostAddress(QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST));
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
