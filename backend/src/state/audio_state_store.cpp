#include "state/audio_state_store.h"

#include "adapters/audio/pulse_audio_state_observer.h"

namespace plasma_bridge::state
{

AudioStateStore::AudioStateStore(QObject *parent)
    : QObject(parent)
{
}

void AudioStateStore::attachObserver(audio::PulseAudioStateObserver *observer)
{
    if (observer == nullptr) {
        return;
    }

    connect(observer, &audio::PulseAudioStateObserver::initialStateReady, this, [this, observer]() {
        updateAudioState(observer->currentState(), observer->hasInitialState(), QStringLiteral("initial"));
    });
    connect(observer,
            &audio::PulseAudioStateObserver::audioStateChanged,
            this,
            [this, observer](const QString &reason, const QString &sinkId, const QString &sourceId) {
                updateAudioState(observer->currentState(), observer->hasInitialState(), reason, sinkId, sourceId);
            });

    if (observer->hasInitialState()) {
        updateAudioState(observer->currentState(), observer->hasInitialState(), QStringLiteral("initial"));
    }
}

void AudioStateStore::updateAudioState(const plasma_bridge::AudioState &state,
                                       const bool ready,
                                       const QString &reason,
                                       const QString &sinkId,
                                       const QString &sourceId)
{
    const bool wasReady = m_ready;
    m_audioState = state;
    m_ready = ready;

    if (wasReady != m_ready) {
        emit readyChanged(m_ready);
    }

    emit audioStateChanged(reason, sinkId, sourceId);
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

std::optional<plasma_bridge::AudioSourceState> AudioStateStore::defaultSource() const
{
    if (!m_ready) {
        return std::nullopt;
    }

    for (const plasma_bridge::AudioSourceState &source : m_audioState.sources) {
        if ((!m_audioState.selectedSourceId.isEmpty() && source.id == m_audioState.selectedSourceId) || source.isDefault) {
            return source;
        }
    }

    return std::nullopt;
}

} // namespace plasma_bridge::state
