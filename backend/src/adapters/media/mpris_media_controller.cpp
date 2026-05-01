#include "adapters/media/mpris_media_controller.h"

#include "state/media_state_store.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>

namespace plasma_bridge::media
{
namespace
{

const QString kMprisObjectPath = QStringLiteral("/org/mpris/MediaPlayer2");
const QString kMprisPlayerInterface = QStringLiteral("org.mpris.MediaPlayer2.Player");

bool actionSupported(const plasma_bridge::MediaPlayerState &player, const control::MediaControlAction action)
{
    if (!player.canControl) {
        return false;
    }

    switch (action) {
    case control::MediaControlAction::Play:
        return player.canPlay;
    case control::MediaControlAction::Pause:
        return player.canPause;
    case control::MediaControlAction::PlayPause:
        return player.canControl;
    case control::MediaControlAction::Next:
        return player.canGoNext;
    case control::MediaControlAction::Previous:
        return player.canGoPrevious;
    case control::MediaControlAction::Seek:
        return player.canSeek;
    }

    return false;
}

QString methodNameForAction(const control::MediaControlAction action)
{
    switch (action) {
    case control::MediaControlAction::Play:
        return QStringLiteral("Play");
    case control::MediaControlAction::Pause:
        return QStringLiteral("Pause");
    case control::MediaControlAction::PlayPause:
        return QStringLiteral("PlayPause");
    case control::MediaControlAction::Next:
        return QStringLiteral("Next");
    case control::MediaControlAction::Previous:
        return QStringLiteral("Previous");
    case control::MediaControlAction::Seek:
        return QStringLiteral("SetPosition");
    }

    return QStringLiteral("PlayPause");
}

std::optional<plasma_bridge::MediaPlayerState> currentPlayerForStore(state::MediaStateStore *mediaStateStore,
                                                                     control::MediaControlResult *result)
{
    if (mediaStateStore == nullptr || !mediaStateStore->isReady()) {
        if (result != nullptr) {
            result->status = control::MediaControlStatus::NotReady;
        }
        return std::nullopt;
    }

    const std::optional<plasma_bridge::MediaPlayerState> currentPlayer = mediaStateStore->currentPlayer();
    if (!currentPlayer.has_value()) {
        if (result != nullptr) {
            result->status = control::MediaControlStatus::NoCurrentPlayer;
        }
        return std::nullopt;
    }

    if (result != nullptr) {
        result->playerId = currentPlayer->playerId;
    }
    return currentPlayer;
}

} // namespace

MprisMediaController::MprisMediaController(state::MediaStateStore *mediaStateStore)
    : m_mediaStateStore(mediaStateStore)
{
}

control::MediaControlResult MprisMediaController::play()
{
    return invoke(control::MediaControlAction::Play);
}

control::MediaControlResult MprisMediaController::pause()
{
    return invoke(control::MediaControlAction::Pause);
}

control::MediaControlResult MprisMediaController::playPause()
{
    return invoke(control::MediaControlAction::PlayPause);
}

control::MediaControlResult MprisMediaController::next()
{
    return invoke(control::MediaControlAction::Next);
}

control::MediaControlResult MprisMediaController::previous()
{
    return invoke(control::MediaControlAction::Previous);
}

control::MediaControlResult MprisMediaController::seek(const qint64 positionMs)
{
    return invokeSeek(positionMs);
}

control::MediaControlResult MprisMediaController::invoke(const control::MediaControlAction action)
{
    control::MediaControlResult result;
    result.action = action;

    const std::optional<plasma_bridge::MediaPlayerState> currentPlayer =
        currentPlayerForStore(m_mediaStateStore, &result);
    if (!currentPlayer.has_value()) {
        return result;
    }

    if (!actionSupported(*currentPlayer, action)) {
        result.status = control::MediaControlStatus::ActionNotSupported;
        return result;
    }

    QDBusInterface playerInterface(currentPlayer->playerId,
                                   kMprisObjectPath,
                                   kMprisPlayerInterface,
                                   QDBusConnection::sessionBus());
    if (!playerInterface.isValid()) {
        result.status = control::MediaControlStatus::PlayerUnavailable;
        return result;
    }

    const QDBusMessage reply = playerInterface.call(methodNameForAction(action));
    result.status = reply.type() == QDBusMessage::ErrorMessage ? control::MediaControlStatus::PlayerUnavailable
                                                               : control::MediaControlStatus::Accepted;
    return result;
}

control::MediaControlResult MprisMediaController::invokeSeek(const qint64 positionMs)
{
    control::MediaControlResult result;
    result.action = control::MediaControlAction::Seek;
    result.positionMs = positionMs;

    const std::optional<plasma_bridge::MediaPlayerState> currentPlayer =
        currentPlayerForStore(m_mediaStateStore, &result);
    if (!currentPlayer.has_value()) {
        return result;
    }

    if (!actionSupported(*currentPlayer, control::MediaControlAction::Seek) || currentPlayer->trackId.isEmpty()) {
        result.status = control::MediaControlStatus::ActionNotSupported;
        return result;
    }

    const qint64 clampedPositionMs =
        currentPlayer->trackLengthMs.has_value() ? qBound<qint64>(0, positionMs, *currentPlayer->trackLengthMs) : qMax<qint64>(0, positionMs);
    result.positionMs = clampedPositionMs;

    QDBusInterface playerInterface(currentPlayer->playerId,
                                   kMprisObjectPath,
                                   kMprisPlayerInterface,
                                   QDBusConnection::sessionBus());
    if (!playerInterface.isValid()) {
        result.status = control::MediaControlStatus::PlayerUnavailable;
        return result;
    }

    const QDBusMessage reply =
        playerInterface.call(QStringLiteral("SetPosition"),
                             QDBusObjectPath(currentPlayer->trackId),
                             static_cast<qlonglong>(clampedPositionMs * 1000));
    result.status = reply.type() == QDBusMessage::ErrorMessage ? control::MediaControlStatus::PlayerUnavailable
                                                               : control::MediaControlStatus::Accepted;
    return result;
}

} // namespace plasma_bridge::media
