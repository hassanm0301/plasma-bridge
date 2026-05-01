#include "api/media_control_http_helpers.h"

namespace plasma_bridge::api
{
namespace
{

const QString kPlayPath = QStringLiteral("/control/media/current/play");
const QString kPausePath = QStringLiteral("/control/media/current/pause");
const QString kPlayPausePath = QStringLiteral("/control/media/current/play-pause");
const QString kNextPath = QStringLiteral("/control/media/current/next");
const QString kPreviousPath = QStringLiteral("/control/media/current/previous");
const QString kSeekPath = QStringLiteral("/control/media/current/seek");

} // namespace

MediaControlRouteParseResult parseMediaControlRoute(const QString &path)
{
    MediaControlRouteParseResult result;

    if (path == kPlayPath) {
        result.match = MediaControlRouteMatch::Match;
        result.route.action = control::MediaControlAction::Play;
        return result;
    }
    if (path == kPausePath) {
        result.match = MediaControlRouteMatch::Match;
        result.route.action = control::MediaControlAction::Pause;
        return result;
    }
    if (path == kPlayPausePath) {
        result.match = MediaControlRouteMatch::Match;
        result.route.action = control::MediaControlAction::PlayPause;
        return result;
    }
    if (path == kNextPath) {
        result.match = MediaControlRouteMatch::Match;
        result.route.action = control::MediaControlAction::Next;
        return result;
    }
    if (path == kPreviousPath) {
        result.match = MediaControlRouteMatch::Match;
        result.route.action = control::MediaControlAction::Previous;
        return result;
    }
    if (path == kSeekPath) {
        result.match = MediaControlRouteMatch::Match;
        result.route.action = control::MediaControlAction::Seek;
        return result;
    }

    return result;
}

int httpStatusCodeForMediaControlStatus(const control::MediaControlStatus status)
{
    switch (status) {
    case control::MediaControlStatus::Accepted:
        return 200;
    case control::MediaControlStatus::NoCurrentPlayer:
        return 404;
    case control::MediaControlStatus::ActionNotSupported:
        return 409;
    case control::MediaControlStatus::NotReady:
    case control::MediaControlStatus::PlayerUnavailable:
        return 503;
    }

    return 503;
}

} // namespace plasma_bridge::api
