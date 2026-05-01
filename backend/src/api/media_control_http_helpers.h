#pragma once

#include "control/media_controller.h"

namespace plasma_bridge::api
{

enum class MediaControlRouteMatch {
    NoMatch,
    Match,
};

struct MediaControlRoute {
    control::MediaControlAction action = control::MediaControlAction::PlayPause;
};

struct MediaControlRouteParseResult {
    MediaControlRouteMatch match = MediaControlRouteMatch::NoMatch;
    MediaControlRoute route;
};

MediaControlRouteParseResult parseMediaControlRoute(const QString &path);
int httpStatusCodeForMediaControlStatus(control::MediaControlStatus status);

} // namespace plasma_bridge::api
