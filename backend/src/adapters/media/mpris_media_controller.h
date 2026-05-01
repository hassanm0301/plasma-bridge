#pragma once

#include "control/media_controller.h"

namespace plasma_bridge::state
{
class MediaStateStore;
}

namespace plasma_bridge::media
{

class MprisMediaController final : public control::MediaController
{
public:
    explicit MprisMediaController(state::MediaStateStore *mediaStateStore);

    control::MediaControlResult play() override;
    control::MediaControlResult pause() override;
    control::MediaControlResult playPause() override;
    control::MediaControlResult next() override;
    control::MediaControlResult previous() override;
    control::MediaControlResult seek(qint64 positionMs) override;

private:
    control::MediaControlResult invoke(control::MediaControlAction action);
    control::MediaControlResult invokeSeek(qint64 positionMs);

    state::MediaStateStore *m_mediaStateStore = nullptr;
};

} // namespace plasma_bridge::media
