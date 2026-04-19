#pragma once

#include "common/audio_state.h"
#include "control/audio_volume_controller.h"
#include "tools/probes/audio_control_probe/audio_control_probe_runner.h"
#include "tools/probes/audio_probe/audio_probe_runner.h"

#include <QJsonObject>
#include <QObject>
#include <QUrl>

namespace plasma_bridge::tests
{

AudioState sampleAudioState();
AudioState alternateAudioState();
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
    void emitAudioStateChanged(const QString &reason, const QString &sinkId, const AudioState &state);
    void emitConnectionFailure(const QString &message);
    int startCount() const;

private:
    AudioState m_state;
    bool m_ready = false;
    int m_startCount = 0;
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

} // namespace plasma_bridge::tests
