#include "state/audio_state_store.h"

#include "adapters/audio/pulse_audio_sink_observer.h"

namespace plasma_bridge::state
{

AudioStateStore::AudioStateStore(QObject *parent)
    : QObject(parent)
{
}

void AudioStateStore::attachObserver(audio::PulseAudioSinkObserver *observer)
{
    if (observer == nullptr) {
        return;
    }

    connect(observer, &audio::PulseAudioSinkObserver::initialStateReady, this, [this, observer]() {
        updateAudioState(observer->currentState(), observer->hasInitialState(), QStringLiteral("initial"));
    });
    connect(observer, &audio::PulseAudioSinkObserver::audioStateChanged, this, [this, observer](const QString &reason, const QString &sinkId) {
        updateAudioState(observer->currentState(), observer->hasInitialState(), reason, sinkId);
    });

    if (observer->hasInitialState()) {
        updateAudioState(observer->currentState(), observer->hasInitialState(), QStringLiteral("initial"));
    }
}

void AudioStateStore::updateAudioState(const plasma_bridge::AudioState &state,
                                       const bool ready,
                                       const QString &reason,
                                       const QString &sinkId)
{
    const bool wasReady = m_ready;
    m_audioState = state;
    m_ready = ready;

    if (wasReady != m_ready) {
        emit readyChanged(m_ready);
    }

    emit audioStateChanged(reason, sinkId);
}

bool AudioStateStore::isReady() const
{
    return m_ready;
}

const plasma_bridge::AudioState &AudioStateStore::audioState() const
{
    return m_audioState;
}

std::optional<plasma_bridge::AudioSinkState> AudioStateStore::defaultSink() const
{
    if (!m_ready) {
        return std::nullopt;
    }

    for (const plasma_bridge::AudioSinkState &sink : m_audioState.sinks) {
        if ((!m_audioState.selectedSinkId.isEmpty() && sink.id == m_audioState.selectedSinkId) || sink.isDefault) {
            return sink;
        }
    }

    return std::nullopt;
}

} // namespace plasma_bridge::state
