#include "common/audio_state.h"
#include "common/media_state.h"
#include "tests/support/test_support.h"

#include <QJsonArray>
#include <QtTest>

class CommonAudioStateTest : public QObject
{
    Q_OBJECT

private slots:
    void deviceJsonUsesNullForEmptyBackend();
    void outputStateJsonUsesNullForEmptySelectedSink();
    void inputStateJsonUsesNullForEmptySelectedSource();
    void fullAudioStateJsonIncludesSources();
    void eventJsonCarriesReasonAndNullDeviceIds();
    void humanReadableEventIncludesImportantFields();
};

class CommonMediaStateTest : public QObject
{
    Q_OBJECT

private slots:
    void playerJsonUsesNullForOptionalFields();
    void stateJsonHandlesMissingPlayer();
    void selectionPrefersPlayingThenLatestActionable();
    void eventJsonCarriesReasonAndPlayerId();
    void humanReadableEventIncludesImportantFields();
};

void CommonAudioStateTest::deviceJsonUsesNullForEmptyBackend()
{
    plasma_bridge::AudioDeviceState device;
    device.id = QStringLiteral("device-1");
    device.label = QStringLiteral("Desk Speakers");
    device.volume = 0.61;
    device.muted = true;
    device.available = false;
    device.isDefault = true;
    device.isVirtual = true;

    const QJsonObject json = plasma_bridge::toJsonObject(device);

    QCOMPARE(json.value(QStringLiteral("id")).toString(), device.id);
    QCOMPARE(json.value(QStringLiteral("label")).toString(), device.label);
    QCOMPARE(json.value(QStringLiteral("volume")).toDouble(), device.volume);
    QCOMPARE(json.value(QStringLiteral("muted")).toBool(), device.muted);
    QCOMPARE(json.value(QStringLiteral("available")).toBool(), device.available);
    QCOMPARE(json.value(QStringLiteral("isDefault")).toBool(), device.isDefault);
    QCOMPARE(json.value(QStringLiteral("isVirtual")).toBool(), device.isVirtual);
    QVERIFY(json.value(QStringLiteral("backendApi")).isNull());
}

void CommonAudioStateTest::outputStateJsonUsesNullForEmptySelectedSink()
{
    plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    state.selectedSinkId.clear();

    const QJsonObject json = plasma_bridge::toJsonObject(plasma_bridge::toAudioOutputState(state));
    const QJsonArray sinks = json.value(QStringLiteral("sinks")).toArray();

    QCOMPARE(sinks.size(), state.sinks.size());
    QVERIFY(json.value(QStringLiteral("selectedSinkId")).isNull());
}

void CommonAudioStateTest::inputStateJsonUsesNullForEmptySelectedSource()
{
    plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    state.selectedSourceId.clear();

    const QJsonObject json = plasma_bridge::toJsonObject(plasma_bridge::toAudioInputState(state));
    const QJsonArray sources = json.value(QStringLiteral("sources")).toArray();

    QCOMPARE(sources.size(), state.sources.size());
    QVERIFY(json.value(QStringLiteral("selectedSourceId")).isNull());
}

void CommonAudioStateTest::fullAudioStateJsonIncludesSources()
{
    const plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    const QJsonObject json = plasma_bridge::toJsonObject(state);

    QCOMPARE(json.value(QStringLiteral("sinks")).toArray().size(), state.sinks.size());
    QCOMPARE(json.value(QStringLiteral("selectedSinkId")).toString(), state.selectedSinkId);
    QCOMPARE(json.value(QStringLiteral("sources")).toArray().size(), state.sources.size());
    QCOMPARE(json.value(QStringLiteral("selectedSourceId")).toString(), state.selectedSourceId);
}

void CommonAudioStateTest::eventJsonCarriesReasonAndNullDeviceIds()
{
    const plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    const QJsonObject json = plasma_bridge::toJsonEventObject(QStringLiteral("initial"), QString(), QString(), state);

    QCOMPARE(json.value(QStringLiteral("event")).toString(), QStringLiteral("audioState"));
    QCOMPARE(json.value(QStringLiteral("reason")).toString(), QStringLiteral("initial"));
    QVERIFY(json.value(QStringLiteral("sinkId")).isNull());
    QVERIFY(json.value(QStringLiteral("sourceId")).isNull());
    QCOMPARE(json.value(QStringLiteral("state")).toObject().value(QStringLiteral("selectedSinkId")).toString(),
             state.selectedSinkId);
    QCOMPARE(json.value(QStringLiteral("state")).toObject().value(QStringLiteral("selectedSourceId")).toString(),
             state.selectedSourceId);
}

void CommonAudioStateTest::humanReadableEventIncludesImportantFields()
{
    const plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    const QString text = plasma_bridge::formatHumanReadableEvent(QStringLiteral("sink-updated"),
                                                                 state.selectedSinkId,
                                                                 QString(),
                                                                 state);

    QVERIFY(text.contains(QStringLiteral("Event: audioState")));
    QVERIFY(text.contains(QStringLiteral("Reason: sink-updated")));
    QVERIFY(text.contains(QStringLiteral("Selected Sink: %1").arg(state.selectedSinkId)));
    QVERIFY(text.contains(QStringLiteral("Selected Source: %1").arg(state.selectedSourceId)));
    QVERIFY(text.contains(QStringLiteral("* USB Audio")));
    QVERIFY(text.contains(QStringLiteral("* USB Microphone")));
    QVERIFY(text.contains(QStringLiteral("backendApi: alsa")));
}

void CommonMediaStateTest::playerJsonUsesNullForOptionalFields()
{
    plasma_bridge::MediaPlayerState player = plasma_bridge::tests::sampleMediaPlayerState();
    player.identity.clear();
    player.desktopEntry.clear();
    player.title.clear();
    player.album.clear();
    player.trackLengthMs.reset();
    player.positionMs.reset();
    player.appIconUrl.clear();
    player.artworkUrl.clear();

    const QJsonObject json = plasma_bridge::toJsonObject(player);

    QCOMPARE(json.value(QStringLiteral("playerId")).toString(), player.playerId);
    QVERIFY(json.value(QStringLiteral("identity")).isNull());
    QVERIFY(json.value(QStringLiteral("desktopEntry")).isNull());
    QCOMPARE(json.value(QStringLiteral("playbackStatus")).toString(), QStringLiteral("playing"));
    QVERIFY(json.value(QStringLiteral("title")).isNull());
    QCOMPARE(json.value(QStringLiteral("artists")).toArray().size(), player.artists.size());
    QVERIFY(json.value(QStringLiteral("album")).isNull());
    QVERIFY(json.value(QStringLiteral("trackLengthMs")).isNull());
    QVERIFY(json.value(QStringLiteral("positionMs")).isNull());
    QCOMPARE(json.value(QStringLiteral("canSeek")).toBool(), player.canSeek);
    QVERIFY(json.value(QStringLiteral("appIconUrl")).isNull());
    QVERIFY(json.value(QStringLiteral("artworkUrl")).isNull());
}

void CommonMediaStateTest::stateJsonHandlesMissingPlayer()
{
    const QJsonObject json = plasma_bridge::toJsonObject(plasma_bridge::tests::sampleMediaStateWithoutPlayer());
    QVERIFY(json.value(QStringLiteral("player")).isNull());
}

void CommonMediaStateTest::selectionPrefersPlayingThenLatestActionable()
{
    plasma_bridge::MediaPlayerState paused = plasma_bridge::tests::alternateMediaPlayerState();
    paused.playbackStatus = plasma_bridge::MediaPlaybackStatus::Paused;
    paused.updateSequence = 9;
    plasma_bridge::MediaPlayerState playing = plasma_bridge::tests::sampleMediaPlayerState();
    playing.updateSequence = 3;
    plasma_bridge::MediaPlayerState nonActionable = plasma_bridge::tests::sampleMediaPlayerState();
    nonActionable.playerId = QStringLiteral("org.mpris.MediaPlayer2.idle");
    nonActionable.playbackStatus = plasma_bridge::MediaPlaybackStatus::Stopped;
    nonActionable.canControl = false;
    nonActionable.updateSequence = 99;

    const std::optional<plasma_bridge::MediaPlayerState> selectedPlaying =
        plasma_bridge::selectCurrentMediaPlayer({paused, playing, nonActionable});
    QVERIFY(selectedPlaying.has_value());
    QCOMPARE(selectedPlaying->playerId, playing.playerId);

    playing.playbackStatus = plasma_bridge::MediaPlaybackStatus::Paused;
    const std::optional<plasma_bridge::MediaPlayerState> selectedFallback =
        plasma_bridge::selectCurrentMediaPlayer({playing, paused, nonActionable});
    QVERIFY(selectedFallback.has_value());
    QCOMPARE(selectedFallback->playerId, paused.playerId);
}

void CommonMediaStateTest::eventJsonCarriesReasonAndPlayerId()
{
    const plasma_bridge::MediaState state = plasma_bridge::tests::sampleMediaState();
    const QJsonObject json =
        plasma_bridge::toJsonEventObject(QStringLiteral("player-updated"), state.player->playerId, state);

    QCOMPARE(json.value(QStringLiteral("event")).toString(), QStringLiteral("mediaState"));
    QCOMPARE(json.value(QStringLiteral("reason")).toString(), QStringLiteral("player-updated"));
    QCOMPARE(json.value(QStringLiteral("playerId")).toString(), state.player->playerId);
    QCOMPARE(json.value(QStringLiteral("state")).toObject().value(QStringLiteral("player")).toObject().value(QStringLiteral("title")).toString(),
             state.player->title);
}

void CommonMediaStateTest::humanReadableEventIncludesImportantFields()
{
    const plasma_bridge::MediaState state = plasma_bridge::tests::sampleMediaState();
    const QString text =
        plasma_bridge::formatHumanReadableEvent(QStringLiteral("player-updated"), state.player->playerId, state);

    QVERIFY(text.contains(QStringLiteral("Event: mediaState")));
    QVERIFY(text.contains(QStringLiteral("Reason: player-updated")));
    QVERIFY(text.contains(QStringLiteral("Current Player: %1").arg(state.player->playerId)));
    QVERIFY(text.contains(QStringLiteral("Playback Status: playing")));
    QVERIFY(text.contains(QStringLiteral("Artists: Artist One, Artist Two")));
    QVERIFY(text.contains(QStringLiteral("Position Ms: 64000")));
    QVERIFY(text.contains(QStringLiteral("Can Seek: yes")));
    QVERIFY(text.contains(QStringLiteral("App Icon Url: /icons/apps/spotify")));
}

int main(int argc, char *argv[])
{
    int status = 0;
    {
        CommonAudioStateTest test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        CommonMediaStateTest test;
        status |= QTest::qExec(&test, argc, argv);
    }
    return status;
}

#include "test_common.moc"
