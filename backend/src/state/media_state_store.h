#pragma once

#include "common/media_state.h"

#include <QObject>

#include <optional>

namespace plasma_bridge::media
{
class MediaObserver;
}

namespace plasma_bridge::state
{

class MediaStateStore final : public QObject
{
    Q_OBJECT

public:
    explicit MediaStateStore(QObject *parent = nullptr);

    void attachObserver(media::MediaObserver *observer);
    void updateMediaState(const plasma_bridge::MediaState &state,
                          bool ready,
                          const QString &reason,
                          const QString &playerId = {});

    bool isReady() const;
    const plasma_bridge::MediaState &mediaState() const;
    std::optional<plasma_bridge::MediaPlayerState> currentPlayer() const;

signals:
    void readyChanged(bool ready);
    void mediaStateChanged(const QString &reason, const QString &playerId);

private:
    plasma_bridge::MediaState m_mediaState;
    bool m_ready = false;
};

} // namespace plasma_bridge::state
