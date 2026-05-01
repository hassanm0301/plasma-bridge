#include "common/media_state.h"

#include <QJsonArray>
#include <QTextStream>

namespace plasma_bridge
{
namespace
{

QJsonValue stringOrNull(const QString &value)
{
    return value.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(value);
}

QJsonValue integerOrNull(const std::optional<qint64> &value)
{
    return value.has_value() ? QJsonValue(*value) : QJsonValue(QJsonValue::Null);
}

QString joinValuesOrFallback(const QStringList &values, const QString &fallback)
{
    return values.isEmpty() ? fallback : values.join(QStringLiteral(", "));
}

QString formatIntegerOrFallback(const std::optional<qint64> &value)
{
    return value.has_value() ? QString::number(*value) : QStringLiteral("(none)");
}

bool isPlayableStatus(const MediaPlaybackStatus status)
{
    return status == MediaPlaybackStatus::Playing;
}

bool isActionablePlayer(const MediaPlayerState &player)
{
    return player.canControl && (player.canPlay || player.canPause || player.canGoNext || player.canGoPrevious);
}

} // namespace

QString mediaPlaybackStatusName(const MediaPlaybackStatus status)
{
    switch (status) {
    case MediaPlaybackStatus::Unknown:
        return QStringLiteral("unknown");
    case MediaPlaybackStatus::Playing:
        return QStringLiteral("playing");
    case MediaPlaybackStatus::Paused:
        return QStringLiteral("paused");
    case MediaPlaybackStatus::Stopped:
        return QStringLiteral("stopped");
    }

    return QStringLiteral("unknown");
}

std::optional<MediaPlayerState> selectCurrentMediaPlayer(const QList<MediaPlayerState> &players)
{
    std::optional<MediaPlayerState> bestPlayingPlayer;
    std::optional<MediaPlayerState> bestFallbackPlayer;

    for (const MediaPlayerState &player : players) {
        if (player.playerId.isEmpty()) {
            continue;
        }

        if (isPlayableStatus(player.playbackStatus)) {
            if (!bestPlayingPlayer.has_value() || player.updateSequence > bestPlayingPlayer->updateSequence) {
                bestPlayingPlayer = player;
            }
            continue;
        }

        if (!isActionablePlayer(player)) {
            continue;
        }

        if (!bestFallbackPlayer.has_value() || player.updateSequence > bestFallbackPlayer->updateSequence) {
            bestFallbackPlayer = player;
        }
    }

    if (bestPlayingPlayer.has_value()) {
        return bestPlayingPlayer;
    }

    return bestFallbackPlayer;
}

QJsonObject toJsonObject(const MediaPlayerState &player)
{
    QJsonArray artists;
    for (const QString &artist : player.artists) {
        artists.append(artist);
    }

    QJsonObject json;
    json[QStringLiteral("playerId")] = player.playerId;
    json[QStringLiteral("identity")] = stringOrNull(player.identity);
    json[QStringLiteral("desktopEntry")] = stringOrNull(player.desktopEntry);
    json[QStringLiteral("playbackStatus")] = mediaPlaybackStatusName(player.playbackStatus);
    json[QStringLiteral("title")] = stringOrNull(player.title);
    json[QStringLiteral("artists")] = artists;
    json[QStringLiteral("album")] = stringOrNull(player.album);
    json[QStringLiteral("trackLengthMs")] = integerOrNull(player.trackLengthMs);
    json[QStringLiteral("positionMs")] = integerOrNull(player.positionMs);
    json[QStringLiteral("canPlay")] = player.canPlay;
    json[QStringLiteral("canPause")] = player.canPause;
    json[QStringLiteral("canGoNext")] = player.canGoNext;
    json[QStringLiteral("canGoPrevious")] = player.canGoPrevious;
    json[QStringLiteral("canControl")] = player.canControl;
    json[QStringLiteral("canSeek")] = player.canSeek;
    json[QStringLiteral("appIconUrl")] = stringOrNull(player.appIconUrl);
    json[QStringLiteral("artworkUrl")] = stringOrNull(player.artworkUrl);
    return json;
}

QJsonObject toJsonObject(const MediaState &state)
{
    QJsonObject json;
    json[QStringLiteral("player")] =
        state.player.has_value() ? QJsonValue(toJsonObject(*state.player)) : QJsonValue(QJsonValue::Null);
    return json;
}

QJsonObject toJsonEventObject(const QString &reason, const QString &playerId, const MediaState &state)
{
    QJsonObject json;
    json[QStringLiteral("event")] = QStringLiteral("mediaState");
    json[QStringLiteral("reason")] = stringOrNull(reason);
    json[QStringLiteral("playerId")] = stringOrNull(playerId);
    json[QStringLiteral("state")] = toJsonObject(state);
    return json;
}

QString formatHumanReadableEvent(const QString &reason, const QString &playerId, const MediaState &state)
{
    QString output;
    QTextStream stream(&output);

    stream << "Event: mediaState\n";
    stream << "Reason: " << (reason.isEmpty() ? QStringLiteral("(none)") : reason) << '\n';
    stream << "Player: " << (playerId.isEmpty() ? QStringLiteral("(none)") : playerId) << '\n';

    if (!state.player.has_value()) {
        stream << "Current Player: (none)\n";
        return output;
    }

    const MediaPlayerState &player = *state.player;
    stream << "Current Player: " << player.playerId << '\n';
    stream << "Identity: " << (player.identity.isEmpty() ? QStringLiteral("(none)") : player.identity) << '\n';
    stream << "Desktop Entry: " << (player.desktopEntry.isEmpty() ? QStringLiteral("(none)") : player.desktopEntry) << '\n';
    stream << "Playback Status: " << mediaPlaybackStatusName(player.playbackStatus) << '\n';
    stream << "Title: " << (player.title.isEmpty() ? QStringLiteral("(none)") : player.title) << '\n';
    stream << "Artists: " << joinValuesOrFallback(player.artists, QStringLiteral("(none)")) << '\n';
    stream << "Album: " << (player.album.isEmpty() ? QStringLiteral("(none)") : player.album) << '\n';
    stream << "Track Length Ms: " << formatIntegerOrFallback(player.trackLengthMs) << '\n';
    stream << "Position Ms: " << formatIntegerOrFallback(player.positionMs) << '\n';
    stream << "Can Play: " << (player.canPlay ? QStringLiteral("yes") : QStringLiteral("no")) << '\n';
    stream << "Can Pause: " << (player.canPause ? QStringLiteral("yes") : QStringLiteral("no")) << '\n';
    stream << "Can Go Next: " << (player.canGoNext ? QStringLiteral("yes") : QStringLiteral("no")) << '\n';
    stream << "Can Go Previous: " << (player.canGoPrevious ? QStringLiteral("yes") : QStringLiteral("no")) << '\n';
    stream << "Can Control: " << (player.canControl ? QStringLiteral("yes") : QStringLiteral("no")) << '\n';
    stream << "Can Seek: " << (player.canSeek ? QStringLiteral("yes") : QStringLiteral("no")) << '\n';
    stream << "App Icon Url: " << (player.appIconUrl.isEmpty() ? QStringLiteral("(none)") : player.appIconUrl) << '\n';
    stream << "Artwork Url: " << (player.artworkUrl.isEmpty() ? QStringLiteral("(none)") : player.artworkUrl) << '\n';

    return output;
}

} // namespace plasma_bridge
