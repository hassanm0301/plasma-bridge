#pragma once

#include "common/audio_state.h"

#include <QObject>

#include <optional>

namespace plasma_bridge::audio
{
class PulseAudioSinkObserver;
}

namespace plasma_bridge::state
{

class AudioStateStore final : public QObject
{
    Q_OBJECT

public:
    explicit AudioStateStore(QObject *parent = nullptr);

    void attachObserver(audio::PulseAudioSinkObserver *observer);
    void updateAudioState(const plasma_bridge::AudioState &state, bool ready, const QString &reason, const QString &sinkId = {});

    bool isReady() const;
    const plasma_bridge::AudioState &audioState() const;
    std::optional<plasma_bridge::AudioSinkState> defaultSink() const;

signals:
    void readyChanged(bool ready);
    void audioStateChanged(const QString &reason, const QString &sinkId);

private:
    plasma_bridge::AudioState m_audioState;
    bool m_ready = false;
};

} // namespace plasma_bridge::state
