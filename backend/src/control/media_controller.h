#pragma once

#include <QJsonObject>
#include <QString>

#include <optional>

namespace plasma_bridge::control
{

enum class MediaControlAction {
    Play,
    Pause,
    PlayPause,
    Next,
    Previous,
    Seek,
};

enum class MediaControlStatus {
    Accepted,
    NotReady,
    NoCurrentPlayer,
    ActionNotSupported,
    PlayerUnavailable,
};

struct MediaControlResult {
    MediaControlStatus status = MediaControlStatus::NotReady;
    MediaControlAction action = MediaControlAction::PlayPause;
    QString playerId;
    std::optional<qint64> positionMs;
};

class MediaController
{
public:
    virtual ~MediaController() = default;

    virtual MediaControlResult play() = 0;
    virtual MediaControlResult pause() = 0;
    virtual MediaControlResult playPause() = 0;
    virtual MediaControlResult next() = 0;
    virtual MediaControlResult previous() = 0;
    virtual MediaControlResult seek(qint64 positionMs) = 0;
};

QString mediaControlActionName(MediaControlAction action);
QString mediaControlStatusName(MediaControlStatus status);
QJsonObject toJsonObject(const MediaControlResult &result);
QString formatHumanReadableResult(const MediaControlResult &result);

} // namespace plasma_bridge::control
