#pragma once

#include "common/audio_state.h"

#include <QObject>

#include <optional>

namespace plasma_bridge::audio
{
class PulseAudioStateObserver;
}

namespace plasma_bridge::state
{

class AudioStateStore final : public QObject
{
    Q_OBJECT

public:
    explicit AudioStateStore(QObject *parent = nullptr);

    void attachObserver(audio::PulseAudioStateObserver *observer);
    void updateAudioState(const plasma_bridge::AudioState &state,
                          bool ready,
                          const QString &reason,
                          const QString &sinkId = {},
                          const QString &sourceId = {});

    bool isReady() const;
    const plasma_bridge::AudioState &audioState() const;
    std::optional<plasma_bridge::AudioSinkState> defaultSink() const;
    std::optional<plasma_bridge::AudioSourceState> defaultSource() const;

signals:
    void readyChanged(bool ready);
    void audioStateChanged(const QString &reason, const QString &sinkId, const QString &sourceId);

private:
    plasma_bridge::AudioState m_audioState;
    bool m_ready = false;
};

} // namespace plasma_bridge::state
