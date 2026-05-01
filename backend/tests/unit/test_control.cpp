#include "control/audio_device_controller.h"
#include "control/audio_volume_controller.h"
#include "control/media_controller.h"
#include "control/window_activation_controller.h"

#include <QtTest>

#include <limits>

class AudioVolumeControllerFormattingTest : public QObject
{
    Q_OBJECT

private slots:
    void statusName_data();
    void statusName();
    void jsonFormattingHandlesNullAndFiniteValues();
    void humanReadableFormattingShowsValueStates();
};

class AudioDeviceControllerFormattingTest : public QObject
{
    Q_OBJECT

private slots:
    void statusName_data();
    void statusName();
    void defaultChangeJsonAndHumanFormatting();
    void muteChangeJsonAndHumanFormatting();
};

class WindowActivationControllerFormattingTest : public QObject
{
    Q_OBJECT

private slots:
    void statusName_data();
    void statusName();
    void activationJsonAndHumanFormatting();
};

class MediaControllerFormattingTest : public QObject
{
    Q_OBJECT

private slots:
    void actionName_data();
    void actionName();
    void statusName_data();
    void statusName();
    void controlJsonAndHumanFormatting();
};

void AudioVolumeControllerFormattingTest::statusName_data()
{
    QTest::addColumn<int>("status");
    QTest::addColumn<QString>("expected");

    QTest::newRow("accepted") << static_cast<int>(plasma_bridge::control::VolumeChangeStatus::Accepted) << QStringLiteral("accepted");
    QTest::newRow("not-ready") << static_cast<int>(plasma_bridge::control::VolumeChangeStatus::NotReady) << QStringLiteral("not_ready");
    QTest::newRow("missing") << static_cast<int>(plasma_bridge::control::VolumeChangeStatus::SinkNotFound) << QStringLiteral("sink_not_found");
    QTest::newRow("not-writable") << static_cast<int>(plasma_bridge::control::VolumeChangeStatus::SinkNotWritable) << QStringLiteral("sink_not_writable");
    QTest::newRow("invalid") << static_cast<int>(plasma_bridge::control::VolumeChangeStatus::InvalidValue) << QStringLiteral("invalid_value");
}

void AudioVolumeControllerFormattingTest::statusName()
{
    QFETCH(int, status);
    QFETCH(QString, expected);

    QCOMPARE(plasma_bridge::control::volumeChangeStatusName(static_cast<plasma_bridge::control::VolumeChangeStatus>(status)),
             expected);
}

void AudioVolumeControllerFormattingTest::jsonFormattingHandlesNullAndFiniteValues()
{
    plasma_bridge::control::VolumeChangeResult result;
    result.status = plasma_bridge::control::VolumeChangeStatus::Accepted;
    result.sinkId = QStringLiteral("sink-1");
    result.requestedValue = std::numeric_limits<double>::quiet_NaN();
    result.targetValue = 0.5;

    const QJsonObject json = plasma_bridge::control::toJsonObject(result);

    QCOMPARE(json.value(QStringLiteral("status")).toString(), QStringLiteral("accepted"));
    QCOMPARE(json.value(QStringLiteral("sinkId")).toString(), QStringLiteral("sink-1"));
    QVERIFY(json.value(QStringLiteral("requestedValue")).isNull());
    QCOMPARE(json.value(QStringLiteral("targetValue")).toDouble(), 0.5);
    QVERIFY(json.value(QStringLiteral("previousValue")).isNull());
}

void AudioVolumeControllerFormattingTest::humanReadableFormattingShowsValueStates()
{
    plasma_bridge::control::VolumeChangeResult result;
    result.status = plasma_bridge::control::VolumeChangeStatus::InvalidValue;
    result.sinkId = QString();
    result.requestedValue = 0.25;
    result.targetValue = std::numeric_limits<double>::infinity();

    const QString text = plasma_bridge::control::formatHumanReadableResult(result);

    QVERIFY(text.contains(QStringLiteral("Status: invalid_value")));
    QVERIFY(text.contains(QStringLiteral("Sink: (none)")));
    QVERIFY(text.contains(QStringLiteral("Requested Value: 0.2500 (25.0%)")));
    QVERIFY(text.contains(QStringLiteral("Target Value: (invalid)")));
    QVERIFY(text.contains(QStringLiteral("Previous Value: (none)")));
}

void AudioDeviceControllerFormattingTest::statusName_data()
{
    QTest::addColumn<int>("status");
    QTest::addColumn<QString>("expected");

    QTest::newRow("accepted")
        << static_cast<int>(plasma_bridge::control::AudioDeviceChangeStatus::Accepted)
        << QStringLiteral("accepted");
    QTest::newRow("not-ready")
        << static_cast<int>(plasma_bridge::control::AudioDeviceChangeStatus::NotReady)
        << QStringLiteral("not_ready");
    QTest::newRow("sink-missing")
        << static_cast<int>(plasma_bridge::control::AudioDeviceChangeStatus::SinkNotFound)
        << QStringLiteral("sink_not_found");
    QTest::newRow("source-missing")
        << static_cast<int>(plasma_bridge::control::AudioDeviceChangeStatus::SourceNotFound)
        << QStringLiteral("source_not_found");
    QTest::newRow("sink-not-writable")
        << static_cast<int>(plasma_bridge::control::AudioDeviceChangeStatus::SinkNotWritable)
        << QStringLiteral("sink_not_writable");
    QTest::newRow("source-not-writable")
        << static_cast<int>(plasma_bridge::control::AudioDeviceChangeStatus::SourceNotWritable)
        << QStringLiteral("source_not_writable");
}

void AudioDeviceControllerFormattingTest::statusName()
{
    QFETCH(int, status);
    QFETCH(QString, expected);

    QCOMPARE(plasma_bridge::control::audioDeviceChangeStatusName(
                 static_cast<plasma_bridge::control::AudioDeviceChangeStatus>(status)),
             expected);
}

void AudioDeviceControllerFormattingTest::defaultChangeJsonAndHumanFormatting()
{
    plasma_bridge::control::DefaultDeviceChangeResult result;
    result.status = plasma_bridge::control::AudioDeviceChangeStatus::Accepted;
    result.deviceId = QStringLiteral("sink-1");

    const QJsonObject json = plasma_bridge::control::toJsonObject(result);
    QCOMPARE(json.value(QStringLiteral("status")).toString(), QStringLiteral("accepted"));
    QCOMPARE(json.value(QStringLiteral("deviceId")).toString(), QStringLiteral("sink-1"));

    const QString text = plasma_bridge::control::formatHumanReadableResult(result);
    QVERIFY(text.contains(QStringLiteral("Status: accepted")));
    QVERIFY(text.contains(QStringLiteral("Device: sink-1")));
}

void AudioDeviceControllerFormattingTest::muteChangeJsonAndHumanFormatting()
{
    plasma_bridge::control::MuteChangeResult result;
    result.status = plasma_bridge::control::AudioDeviceChangeStatus::SourceNotWritable;
    result.deviceId = QString();
    result.requestedMuted = true;
    result.targetMuted = false;

    const QJsonObject json = plasma_bridge::control::toJsonObject(result);
    QCOMPARE(json.value(QStringLiteral("status")).toString(), QStringLiteral("source_not_writable"));
    QCOMPARE(json.value(QStringLiteral("deviceId")).toString(), QString());
    QCOMPARE(json.value(QStringLiteral("requestedMuted")).toBool(), true);
    QCOMPARE(json.value(QStringLiteral("targetMuted")).toBool(), false);
    QVERIFY(json.value(QStringLiteral("previousMuted")).isNull());

    const QString text = plasma_bridge::control::formatHumanReadableResult(result);
    QVERIFY(text.contains(QStringLiteral("Status: source_not_writable")));
    QVERIFY(text.contains(QStringLiteral("Device: (none)")));
    QVERIFY(text.contains(QStringLiteral("Requested Muted: true")));
    QVERIFY(text.contains(QStringLiteral("Target Muted: false")));
    QVERIFY(text.contains(QStringLiteral("Previous Muted: (none)")));
}

void WindowActivationControllerFormattingTest::statusName_data()
{
    QTest::addColumn<int>("status");
    QTest::addColumn<QString>("expected");

    QTest::newRow("accepted")
        << static_cast<int>(plasma_bridge::control::WindowActivationStatus::Accepted)
        << QStringLiteral("accepted");
    QTest::newRow("not-ready")
        << static_cast<int>(plasma_bridge::control::WindowActivationStatus::NotReady)
        << QStringLiteral("not_ready");
    QTest::newRow("window-missing")
        << static_cast<int>(plasma_bridge::control::WindowActivationStatus::WindowNotFound)
        << QStringLiteral("window_not_found");
    QTest::newRow("window-not-activatable")
        << static_cast<int>(plasma_bridge::control::WindowActivationStatus::WindowNotActivatable)
        << QStringLiteral("window_not_activatable");
}

void WindowActivationControllerFormattingTest::statusName()
{
    QFETCH(int, status);
    QFETCH(QString, expected);

    QCOMPARE(plasma_bridge::control::windowActivationStatusName(
                 static_cast<plasma_bridge::control::WindowActivationStatus>(status)),
             expected);
}

void WindowActivationControllerFormattingTest::activationJsonAndHumanFormatting()
{
    plasma_bridge::control::WindowActivationResult result;
    result.status = plasma_bridge::control::WindowActivationStatus::WindowNotActivatable;
    result.windowId = QStringLiteral("window-editor");

    const QJsonObject json = plasma_bridge::control::toJsonObject(result);
    QCOMPARE(json.value(QStringLiteral("status")).toString(), QStringLiteral("window_not_activatable"));
    QCOMPARE(json.value(QStringLiteral("windowId")).toString(), QStringLiteral("window-editor"));

    const QString text = plasma_bridge::control::formatHumanReadableResult(result);
    QVERIFY(text.contains(QStringLiteral("Status: window_not_activatable")));
    QVERIFY(text.contains(QStringLiteral("Window: window-editor")));
}

void MediaControllerFormattingTest::actionName_data()
{
    QTest::addColumn<int>("action");
    QTest::addColumn<QString>("expected");

    QTest::newRow("play") << static_cast<int>(plasma_bridge::control::MediaControlAction::Play) << QStringLiteral("play");
    QTest::newRow("pause") << static_cast<int>(plasma_bridge::control::MediaControlAction::Pause) << QStringLiteral("pause");
    QTest::newRow("play-pause")
        << static_cast<int>(plasma_bridge::control::MediaControlAction::PlayPause) << QStringLiteral("play_pause");
    QTest::newRow("next") << static_cast<int>(plasma_bridge::control::MediaControlAction::Next) << QStringLiteral("next");
    QTest::newRow("previous")
        << static_cast<int>(plasma_bridge::control::MediaControlAction::Previous) << QStringLiteral("previous");
    QTest::newRow("seek") << static_cast<int>(plasma_bridge::control::MediaControlAction::Seek) << QStringLiteral("seek");
}

void MediaControllerFormattingTest::actionName()
{
    QFETCH(int, action);
    QFETCH(QString, expected);

    QCOMPARE(plasma_bridge::control::mediaControlActionName(
                 static_cast<plasma_bridge::control::MediaControlAction>(action)),
             expected);
}

void MediaControllerFormattingTest::statusName_data()
{
    QTest::addColumn<int>("status");
    QTest::addColumn<QString>("expected");

    QTest::newRow("accepted")
        << static_cast<int>(plasma_bridge::control::MediaControlStatus::Accepted) << QStringLiteral("accepted");
    QTest::newRow("not-ready")
        << static_cast<int>(plasma_bridge::control::MediaControlStatus::NotReady) << QStringLiteral("not_ready");
    QTest::newRow("no-current-player")
        << static_cast<int>(plasma_bridge::control::MediaControlStatus::NoCurrentPlayer)
        << QStringLiteral("no_current_player");
    QTest::newRow("action-not-supported")
        << static_cast<int>(plasma_bridge::control::MediaControlStatus::ActionNotSupported)
        << QStringLiteral("action_not_supported");
    QTest::newRow("player-unavailable")
        << static_cast<int>(plasma_bridge::control::MediaControlStatus::PlayerUnavailable)
        << QStringLiteral("player_unavailable");
}

void MediaControllerFormattingTest::statusName()
{
    QFETCH(int, status);
    QFETCH(QString, expected);

    QCOMPARE(plasma_bridge::control::mediaControlStatusName(
                 static_cast<plasma_bridge::control::MediaControlStatus>(status)),
             expected);
}

void MediaControllerFormattingTest::controlJsonAndHumanFormatting()
{
    plasma_bridge::control::MediaControlResult result;
    result.status = plasma_bridge::control::MediaControlStatus::ActionNotSupported;
    result.action = plasma_bridge::control::MediaControlAction::PlayPause;
    result.playerId = QStringLiteral("org.mpris.MediaPlayer2.spotify");
    result.positionMs = 88000;

    const QJsonObject json = plasma_bridge::control::toJsonObject(result);
    QCOMPARE(json.value(QStringLiteral("status")).toString(), QStringLiteral("action_not_supported"));
    QCOMPARE(json.value(QStringLiteral("action")).toString(), QStringLiteral("play_pause"));
    QCOMPARE(json.value(QStringLiteral("playerId")).toString(), QStringLiteral("org.mpris.MediaPlayer2.spotify"));
    QCOMPARE(json.value(QStringLiteral("positionMs")).toInteger(), 88000);

    const QString text = plasma_bridge::control::formatHumanReadableResult(result);
    QVERIFY(text.contains(QStringLiteral("Status: action_not_supported")));
    QVERIFY(text.contains(QStringLiteral("Action: play_pause")));
    QVERIFY(text.contains(QStringLiteral("Player: org.mpris.MediaPlayer2.spotify")));
    QVERIFY(text.contains(QStringLiteral("Position Ms: 88000")));
}

int main(int argc, char *argv[])
{
    int status = 0;
    {
        AudioVolumeControllerFormattingTest test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        AudioDeviceControllerFormattingTest test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        WindowActivationControllerFormattingTest test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        MediaControllerFormattingTest test;
        status |= QTest::qExec(&test, argc, argv);
    }
    return status;
}

#include "test_control.moc"
