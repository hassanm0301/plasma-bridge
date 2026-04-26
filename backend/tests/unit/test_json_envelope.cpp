#include "api/json_envelope.h"
#include "control/audio_device_controller.h"
#include "control/audio_volume_controller.h"

#include <QtTest>

class JsonEnvelopeTest : public QObject
{
    Q_OBJECT

private slots:
    void httpSuccessEnvelopeWrapsPayload();
    void httpErrorEnvelopeWrapsErrorDetails();
    void successPayloadsOmitLegacyStatus();
    void errorDetailsPreserveOptionalFields();
};

void JsonEnvelopeTest::httpSuccessEnvelopeWrapsPayload()
{
    const QJsonObject envelope = plasma_bridge::api::buildHttpSuccessEnvelope(
                                     QJsonObject{{QStringLiteral("deviceId"), QStringLiteral("sink-1")}})
                                     .object();

    QCOMPARE(envelope.value(QStringLiteral("payload")).toObject().value(QStringLiteral("deviceId")).toString(),
             QStringLiteral("sink-1"));
    QVERIFY(envelope.value(QStringLiteral("error")).isNull());
}

void JsonEnvelopeTest::httpErrorEnvelopeWrapsErrorDetails()
{
    const QJsonObject envelope = plasma_bridge::api::buildHttpErrorEnvelope(
                                     QStringLiteral("sink_not_found"),
                                     QStringLiteral("Requested sink was not found."),
                                     QJsonObject{{QStringLiteral("deviceId"), QStringLiteral("sink-1")}})
                                     .object();
    const QJsonObject error = envelope.value(QStringLiteral("error")).toObject();

    QVERIFY(envelope.value(QStringLiteral("payload")).isNull());
    QCOMPARE(error.value(QStringLiteral("code")).toString(), QStringLiteral("sink_not_found"));
    QCOMPARE(error.value(QStringLiteral("message")).toString(), QStringLiteral("Requested sink was not found."));
    QCOMPARE(error.value(QStringLiteral("details")).toObject().value(QStringLiteral("deviceId")).toString(),
             QStringLiteral("sink-1"));
}

void JsonEnvelopeTest::successPayloadsOmitLegacyStatus()
{
    plasma_bridge::control::DefaultDeviceChangeResult defaultResult;
    defaultResult.deviceId = QStringLiteral("sink-1");

    const QJsonObject defaultPayload = plasma_bridge::api::buildDefaultDevicePayload(defaultResult);
    QCOMPARE(defaultPayload.value(QStringLiteral("deviceId")).toString(), QStringLiteral("sink-1"));
    QVERIFY(!defaultPayload.contains(QStringLiteral("status")));

    plasma_bridge::control::VolumeChangeResult volumeResult;
    volumeResult.sinkId = QStringLiteral("sink-1");
    volumeResult.requestedValue = 0.25;
    volumeResult.targetValue = 0.25;
    volumeResult.previousValue = 0.5;

    const QJsonObject volumePayload = plasma_bridge::api::buildVolumePayload(volumeResult);
    QCOMPARE(volumePayload.value(QStringLiteral("sinkId")).toString(), QStringLiteral("sink-1"));
    QCOMPARE(volumePayload.value(QStringLiteral("requestedValue")).toDouble(), 0.25);
    QCOMPARE(volumePayload.value(QStringLiteral("targetValue")).toDouble(), 0.25);
    QCOMPARE(volumePayload.value(QStringLiteral("previousValue")).toDouble(), 0.5);
    QVERIFY(!volumePayload.contains(QStringLiteral("status")));
}

void JsonEnvelopeTest::errorDetailsPreserveOptionalFields()
{
    plasma_bridge::control::MuteChangeResult muteResult;
    muteResult.deviceId = QStringLiteral("source-1");
    muteResult.requestedMuted = false;
    muteResult.targetMuted = true;

    const QJsonObject muteDetails = plasma_bridge::api::buildMuteErrorDetails(muteResult);
    QCOMPARE(muteDetails.value(QStringLiteral("deviceId")).toString(), QStringLiteral("source-1"));
    QCOMPARE(muteDetails.value(QStringLiteral("requestedMuted")).toBool(), false);
    QCOMPARE(muteDetails.value(QStringLiteral("targetMuted")).toBool(), true);
    QVERIFY(muteDetails.value(QStringLiteral("previousMuted")).isNull());
}

QTEST_GUILESS_MAIN(JsonEnvelopeTest)

#include "test_json_envelope.moc"
