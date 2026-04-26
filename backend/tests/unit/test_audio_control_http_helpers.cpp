#include "api/audio_control_http_helpers.h"

#include <QtTest>

class AudioControlHttpHelpersTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesRoutesAndRejectsInvalidIds();
    void mapsStatusesToHttpCodes();
};

void AudioControlHttpHelpersTest::parsesRoutesAndRejectsInvalidIds()
{
    using namespace plasma_bridge::api;

    const AudioControlRouteParseResult sinkVolume =
        parseAudioControlRoute(QStringLiteral("/control/audio/sinks/sink-1/volume"));
    QCOMPARE(sinkVolume.match, AudioControlRouteMatch::Match);
    QCOMPARE(sinkVolume.route.targetKind, AudioControlTargetKind::Sink);
    QCOMPARE(sinkVolume.route.operation, AudioControlOperation::SetVolume);
    QCOMPARE(sinkVolume.route.deviceId, QStringLiteral("sink-1"));

    const AudioControlRouteParseResult sourceDefault =
        parseAudioControlRoute(QStringLiteral("/control/audio/sources/source-1/default"));
    QCOMPARE(sourceDefault.match, AudioControlRouteMatch::Match);
    QCOMPARE(sourceDefault.route.targetKind, AudioControlTargetKind::Source);
    QCOMPARE(sourceDefault.route.operation, AudioControlOperation::SetDefault);
    QCOMPARE(sourceDefault.route.deviceId, QStringLiteral("source-1"));

    const AudioControlRouteParseResult sourceMute =
        parseAudioControlRoute(QStringLiteral("/control/audio/sources/source-1/mute"));
    QCOMPARE(sourceMute.match, AudioControlRouteMatch::Match);
    QCOMPARE(sourceMute.route.targetKind, AudioControlTargetKind::Source);
    QCOMPARE(sourceMute.route.operation, AudioControlOperation::SetMute);
    QCOMPARE(sourceMute.route.deviceId, QStringLiteral("source-1"));

    const AudioControlRouteParseResult invalidSink =
        parseAudioControlRoute(QStringLiteral("/control/audio/sinks/sink%2Fpart/default"));
    QCOMPARE(invalidSink.match, AudioControlRouteMatch::InvalidSinkId);

    const AudioControlRouteParseResult invalidSource =
        parseAudioControlRoute(QStringLiteral("/control/audio/sources/source%2Fpart/mute"));
    QCOMPARE(invalidSource.match, AudioControlRouteMatch::InvalidSourceId);

    const AudioControlRouteParseResult noMatch =
        parseAudioControlRoute(QStringLiteral("/control/audio/sources/source-1/volume"));
    QCOMPARE(noMatch.match, AudioControlRouteMatch::NoMatch);
}

void AudioControlHttpHelpersTest::mapsStatusesToHttpCodes()
{
    QCOMPARE(plasma_bridge::api::httpStatusCodeForVolumeChangeStatus(plasma_bridge::control::VolumeChangeStatus::Accepted), 200);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForVolumeChangeStatus(plasma_bridge::control::VolumeChangeStatus::InvalidValue), 400);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForVolumeChangeStatus(plasma_bridge::control::VolumeChangeStatus::SinkNotFound), 404);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForVolumeChangeStatus(plasma_bridge::control::VolumeChangeStatus::SinkNotWritable),
             409);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForVolumeChangeStatus(plasma_bridge::control::VolumeChangeStatus::NotReady), 503);

    QCOMPARE(plasma_bridge::api::httpStatusCodeForAudioDeviceChangeStatus(
                 plasma_bridge::control::AudioDeviceChangeStatus::Accepted),
             200);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForAudioDeviceChangeStatus(
                 plasma_bridge::control::AudioDeviceChangeStatus::SinkNotFound),
             404);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForAudioDeviceChangeStatus(
                 plasma_bridge::control::AudioDeviceChangeStatus::SourceNotFound),
             404);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForAudioDeviceChangeStatus(
                 plasma_bridge::control::AudioDeviceChangeStatus::SinkNotWritable),
             409);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForAudioDeviceChangeStatus(
                 plasma_bridge::control::AudioDeviceChangeStatus::SourceNotWritable),
             409);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForAudioDeviceChangeStatus(
                 plasma_bridge::control::AudioDeviceChangeStatus::NotReady),
             503);

    QCOMPARE(plasma_bridge::api::reasonPhraseForStatusCode(200), QByteArrayLiteral("OK"));
    QCOMPARE(plasma_bridge::api::reasonPhraseForStatusCode(409), QByteArrayLiteral("Conflict"));
}

QTEST_GUILESS_MAIN(AudioControlHttpHelpersTest)

#include "test_audio_control_http_helpers.moc"
