#pragma once

#include "common/audio_state.h"
#include "common/media_state.h"
#include "common/window_state.h"
#include "control/audio_device_controller.h"
#include "control/audio_volume_controller.h"
#include "control/media_controller.h"
#include "tools/probes/audio_control_probe/audio_control_probe_runner.h"
#include "tools/probes/audio_probe/audio_probe_runner.h"
#include "tools/probes/window_probe/window_probe_runner.h"

#include <QJsonObject>
#include <QObject>
#include <QUrl>

namespace plasma_bridge::tests
{

AudioState sampleAudioState();
AudioState alternateAudioState();
MediaPlayerState sampleMediaPlayerState();
MediaPlayerState alternateMediaPlayerState();
MediaState sampleMediaState();
MediaState sampleMediaStateWithoutPlayer();
WindowSnapshot sampleWindowSnapshot();
WindowSnapshot sampleWindowSnapshotWithoutActiveWindow();
void ensureDocsResourcesInitialized();
QJsonObject parseJsonObject(const QByteArray &json);
QUrl httpUrl(quint16 port, const QString &path);
QUrl wsUrl(quint16 port, const QString &path);

class FakeAudioProbeSource final : public tools::audio_probe::AudioProbeSource
{
    Q_OBJECT

public:
    explicit FakeAudioProbeSource(QObject *parent = nullptr);

    void start() override;
    const AudioState &currentState() const override;
    bool hasInitialState() const override;

    void setState(const AudioState &state, bool ready);
    void emitInitialStateReady(const AudioState &state);
    void emitAudioStateChanged(const QString &reason,
                               const QString &sinkId,
                               const QString &sourceId,
                               const AudioState &state);
    void emitConnectionFailure(const QString &message);
    int startCount() const;

private:
    AudioState m_state;
    bool m_ready = false;
    int m_startCount = 0;
};

class FakeWindowProbeSource final : public tools::window_probe::WindowProbeSource
{
    Q_OBJECT

public:
    explicit FakeWindowProbeSource(QObject *parent = nullptr);

    void start() override;
    const WindowSnapshot &currentSnapshot() const override;
    bool hasInitialSnapshot() const override;
    QString backendName() const override;

    void setSnapshot(const WindowSnapshot &snapshot, bool ready);
    void emitInitialSnapshotReady(const WindowSnapshot &snapshot);
    void emitConnectionFailure(const QString &message);
    int startCount() const;

private:
    WindowSnapshot m_snapshot;
    bool m_ready = false;
    int m_startCount = 0;
};

class FakeMediaSource final : public QObject
{
    Q_OBJECT

public:
    explicit FakeMediaSource(QObject *parent = nullptr);

    void setState(const MediaState &state, bool ready);
    const MediaState &currentState() const;
    bool hasInitialState() const;

    void emitInitialStateReady(const MediaState &state);
    void emitMediaStateChanged(const QString &reason, const QString &playerId, const MediaState &state);
    void emitConnectionFailure(const QString &message);

signals:
    void initialStateReady();
    void mediaStateChanged(const QString &reason, const QString &playerId);
    void connectionFailed(const QString &message);

private:
    MediaState m_state;
    bool m_ready = false;
};

class FakeWindowProbeBackendController final : public tools::window_probe::WindowProbeBackendController
{
    Q_OBJECT

public:
    explicit FakeWindowProbeBackendController(QObject *parent = nullptr);

    tools::window_probe::WindowProbeCommandResult setup() override;
    tools::window_probe::WindowProbeCommandResult status() override;
    tools::window_probe::WindowProbeCommandResult teardown() override;
    control::WindowActivationResult activateWindow(const QString &windowId) override;

    void setSetupResult(const tools::window_probe::WindowProbeCommandResult &result);
    void setStatusResult(const tools::window_probe::WindowProbeCommandResult &result);
    void setTeardownResult(const tools::window_probe::WindowProbeCommandResult &result);
    void setActivationResult(const control::WindowActivationResult &result);
    QString lastActivationWindowId() const;
    int activationCallCount() const;

private:
    tools::window_probe::WindowProbeCommandResult m_setupResult;
    tools::window_probe::WindowProbeCommandResult m_statusResult;
    tools::window_probe::WindowProbeCommandResult m_teardownResult;
    control::WindowActivationResult m_activationResult;
    QString m_lastActivationWindowId;
    int m_activationCallCount = 0;
};

class FakeSubmissionGate final : public tools::audio_control_probe::AudioControlSubmissionGate
{
    Q_OBJECT

public:
    explicit FakeSubmissionGate(QObject *parent = nullptr);

    bool shouldSubmitRequest() const override;
    void setShouldSubmitRequest(bool value);

private:
    bool m_shouldSubmitRequest = false;
};

class FakeAudioVolumeController final : public control::AudioVolumeController
{
public:
    enum class Operation {
        None,
        Set,
        Increment,
        Decrement,
    };

    void setResult(Operation operation, const control::VolumeChangeResult &result);

    control::VolumeChangeResult setVolume(const QString &sinkId, double value) override;
    control::VolumeChangeResult incrementVolume(const QString &sinkId, double value) override;
    control::VolumeChangeResult decrementVolume(const QString &sinkId, double value) override;

    Operation lastOperation() const;
    QString lastSinkId() const;
    double lastValue() const;
    int callCount() const;

private:
    control::VolumeChangeResult invoke(Operation operation, const QString &sinkId, double value);

    control::VolumeChangeResult m_setResult;
    control::VolumeChangeResult m_incrementResult;
    control::VolumeChangeResult m_decrementResult;
    Operation m_lastOperation = Operation::None;
    QString m_lastSinkId;
    double m_lastValue = 0.0;
    int m_callCount = 0;
};

class FakeAudioDeviceController final : public control::AudioDeviceController
{
public:
    enum class Operation {
        None,
        SetDefaultSink,
        SetDefaultSource,
        SetSinkMute,
        SetSourceMute,
    };

    void setDefaultResult(Operation operation, const control::DefaultDeviceChangeResult &result);
    void setMuteResult(Operation operation, const control::MuteChangeResult &result);

    control::DefaultDeviceChangeResult setDefaultSink(const QString &sinkId) override;
    control::DefaultDeviceChangeResult setDefaultSource(const QString &sourceId) override;
    control::MuteChangeResult setSinkMuted(const QString &sinkId, bool muted) override;
    control::MuteChangeResult setSourceMuted(const QString &sourceId, bool muted) override;

    Operation lastOperation() const;
    QString lastDeviceId() const;
    std::optional<bool> lastMuted() const;
    int callCount() const;

private:
    control::DefaultDeviceChangeResult invokeDefault(Operation operation, const QString &deviceId);
    control::MuteChangeResult invokeMute(Operation operation, const QString &deviceId, bool muted);

    control::DefaultDeviceChangeResult m_setDefaultSinkResult;
    control::DefaultDeviceChangeResult m_setDefaultSourceResult;
    control::MuteChangeResult m_setSinkMuteResult;
    control::MuteChangeResult m_setSourceMuteResult;
    Operation m_lastOperation = Operation::None;
    QString m_lastDeviceId;
    std::optional<bool> m_lastMuted;
    int m_callCount = 0;
};

class FakeWindowActivationController final : public control::WindowActivationController
{
public:
    void setResult(const control::WindowActivationResult &result);

    control::WindowActivationResult activateWindow(const QString &windowId) override;

    QString lastWindowId() const;
    int callCount() const;

private:
    control::WindowActivationResult m_result;
    QString m_lastWindowId;
    int m_callCount = 0;
};

class FakeMediaController final : public control::MediaController
{
public:
    using Operation = control::MediaControlAction;

    void setResult(Operation operation, const control::MediaControlResult &result);

    control::MediaControlResult play() override;
    control::MediaControlResult pause() override;
    control::MediaControlResult playPause() override;
    control::MediaControlResult next() override;
    control::MediaControlResult previous() override;
    control::MediaControlResult seek(qint64 positionMs) override;

    Operation lastOperation() const;
    QString lastPlayerId() const;
    std::optional<qint64> lastPositionMs() const;
    int callCount() const;

private:
    control::MediaControlResult invoke(Operation operation, std::optional<qint64> positionMs = std::nullopt);

    control::MediaControlResult m_playResult;
    control::MediaControlResult m_pauseResult;
    control::MediaControlResult m_playPauseResult;
    control::MediaControlResult m_nextResult;
    control::MediaControlResult m_previousResult;
    control::MediaControlResult m_seekResult;
    Operation m_lastOperation = Operation::PlayPause;
    QString m_lastPlayerId;
    std::optional<qint64> m_lastPositionMs;
    int m_callCount = 0;
};

} // namespace plasma_bridge::tests
