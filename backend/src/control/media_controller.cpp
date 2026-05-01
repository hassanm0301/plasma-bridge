#include "control/media_controller.h"

#include <QTextStream>

namespace plasma_bridge::control
{

QString mediaControlActionName(const MediaControlAction action)
{
    switch (action) {
    case MediaControlAction::Play:
        return QStringLiteral("play");
    case MediaControlAction::Pause:
        return QStringLiteral("pause");
    case MediaControlAction::PlayPause:
        return QStringLiteral("play_pause");
    case MediaControlAction::Next:
        return QStringLiteral("next");
    case MediaControlAction::Previous:
        return QStringLiteral("previous");
    case MediaControlAction::Seek:
        return QStringLiteral("seek");
    }

    return QStringLiteral("play_pause");
}

QString mediaControlStatusName(const MediaControlStatus status)
{
    switch (status) {
    case MediaControlStatus::Accepted:
        return QStringLiteral("accepted");
    case MediaControlStatus::NotReady:
        return QStringLiteral("not_ready");
    case MediaControlStatus::NoCurrentPlayer:
        return QStringLiteral("no_current_player");
    case MediaControlStatus::ActionNotSupported:
        return QStringLiteral("action_not_supported");
    case MediaControlStatus::PlayerUnavailable:
        return QStringLiteral("player_unavailable");
    }

    return QStringLiteral("not_ready");
}

QJsonObject toJsonObject(const MediaControlResult &result)
{
    QJsonObject json;
    json[QStringLiteral("status")] = mediaControlStatusName(result.status);
    json[QStringLiteral("action")] = mediaControlActionName(result.action);
    json[QStringLiteral("playerId")] = result.playerId.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(result.playerId);
    json[QStringLiteral("positionMs")] =
        result.positionMs.has_value() ? QJsonValue(*result.positionMs) : QJsonValue(QJsonValue::Null);
    return json;
}

QString formatHumanReadableResult(const MediaControlResult &result)
{
    QString output;
    QTextStream stream(&output);

    stream << "Status: " << mediaControlStatusName(result.status) << '\n';
    stream << "Action: " << mediaControlActionName(result.action) << '\n';
    stream << "Player: " << (result.playerId.isEmpty() ? QStringLiteral("(none)") : result.playerId) << '\n';
    stream << "Position Ms: "
           << (result.positionMs.has_value() ? QString::number(*result.positionMs) : QStringLiteral("(none)")) << '\n';

    return output;
}

} // namespace plasma_bridge::control
