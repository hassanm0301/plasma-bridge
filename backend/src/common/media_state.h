#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

namespace plasma_bridge
{

enum class MediaPlaybackStatus {
    Unknown,
    Playing,
    Paused,
    Stopped,
};

struct MediaPlayerState {
    QString playerId;
    QString identity;
    QString desktopEntry;
    MediaPlaybackStatus playbackStatus = MediaPlaybackStatus::Unknown;
    QString title;
    QStringList artists;
    QString album;
    std::optional<qint64> trackLengthMs;
    std::optional<qint64> positionMs;
    bool canPlay = false;
    bool canPause = false;
    bool canGoNext = false;
    bool canGoPrevious = false;
    bool canControl = false;
    bool canSeek = false;
    QString appIconUrl;
    QString artworkUrl;
    QString trackId;
    quint64 updateSequence = 0;
};

struct MediaState {
    std::optional<MediaPlayerState> player;
};

QString mediaPlaybackStatusName(MediaPlaybackStatus status);
std::optional<MediaPlayerState> selectCurrentMediaPlayer(const QList<MediaPlayerState> &players);

QJsonObject toJsonObject(const MediaPlayerState &player);
QJsonObject toJsonObject(const MediaState &state);
QJsonObject toJsonEventObject(const QString &reason, const QString &playerId, const MediaState &state);

QString formatHumanReadableEvent(const QString &reason, const QString &playerId, const MediaState &state);

} // namespace plasma_bridge
