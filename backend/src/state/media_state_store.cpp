#include "state/media_state_store.h"

#include "adapters/media/media_observer.h"

namespace plasma_bridge::state
{

MediaStateStore::MediaStateStore(QObject *parent)
    : QObject(parent)
{
}

void MediaStateStore::attachObserver(media::MediaObserver *observer)
{
    if (observer == nullptr) {
        return;
    }

    connect(observer, &media::MediaObserver::initialStateReady, this, [this, observer]() {
        updateMediaState(observer->currentState(), observer->hasInitialState(), QStringLiteral("initial"));
    });
    connect(observer, &media::MediaObserver::mediaStateChanged, this, [this, observer](const QString &reason, const QString &playerId) {
        updateMediaState(observer->currentState(), observer->hasInitialState(), reason, playerId);
    });

    if (observer->hasInitialState()) {
        updateMediaState(observer->currentState(), observer->hasInitialState(), QStringLiteral("initial"));
    }
}

void MediaStateStore::updateMediaState(const plasma_bridge::MediaState &state,
                                       const bool ready,
                                       const QString &reason,
                                       const QString &playerId)
{
    const bool wasReady = m_ready;
    m_mediaState = state;
    m_ready = ready;

    if (wasReady != m_ready) {
        emit readyChanged(m_ready);
    }

    emit mediaStateChanged(reason, playerId);
}

bool MediaStateStore::isReady() const
{
    return m_ready;
}

const plasma_bridge::MediaState &MediaStateStore::mediaState() const
{
    return m_mediaState;
}

std::optional<plasma_bridge::MediaPlayerState> MediaStateStore::currentPlayer() const
{
    if (!m_ready) {
        return std::nullopt;
    }

    return m_mediaState.player;
}

} // namespace plasma_bridge::state
