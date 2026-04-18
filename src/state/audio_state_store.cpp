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
        syncFromObserver(observer, QStringLiteral("initial"), QString());
    });
    connect(observer, &audio::PulseAudioSinkObserver::audioStateChanged, this, [this, observer](const QString &reason, const QString &sinkId) {
        syncFromObserver(observer, reason, sinkId);
    });

    if (observer->hasInitialState()) {
        syncFromObserver(observer, QStringLiteral("initial"), QString());
    }
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

void AudioStateStore::syncFromObserver(audio::PulseAudioSinkObserver *observer, const QString &reason, const QString &sinkId)
{
    if (observer == nullptr) {
        return;
    }

    const bool wasReady = m_ready;
    m_audioState = observer->currentState();
    m_ready = observer->hasInitialState();

    if (wasReady != m_ready) {
        emit readyChanged(m_ready);
    }

    emit audioStateChanged(reason, sinkId);
}

} // namespace plasma_bridge::state
