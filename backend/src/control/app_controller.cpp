#include "control/app_controller.h"

#include <QTextStream>

namespace plasma_bridge::control
{
namespace
{

QString formatAppId(const QString &appId)
{
    return appId.isEmpty() ? QStringLiteral("(none)") : appId;
}

} // namespace

QString favoriteAppsStatusName(const FavoriteAppsStatus status)
{
    switch (status) {
    case FavoriteAppsStatus::Accepted:
        return QStringLiteral("accepted");
    case FavoriteAppsStatus::StorageError:
        return QStringLiteral("storage_error");
    }

    return QStringLiteral("storage_error");
}

QString appFavoriteChangeStatusName(const AppFavoriteChangeStatus status)
{
    switch (status) {
    case AppFavoriteChangeStatus::Accepted:
        return QStringLiteral("accepted");
    case AppFavoriteChangeStatus::AppNotFound:
        return QStringLiteral("app_not_found");
    case AppFavoriteChangeStatus::StorageError:
        return QStringLiteral("storage_error");
    }

    return QStringLiteral("storage_error");
}

QJsonObject toJsonObject(const AppFavoriteChangeResult &result)
{
    QJsonObject json;
    json[QStringLiteral("status")] = appFavoriteChangeStatusName(result.status);
    json[QStringLiteral("appId")] = result.appId;
    json[QStringLiteral("favorite")] = result.favorite;
    return json;
}

QString formatHumanReadableResult(const AppFavoriteChangeResult &result)
{
    QString output;
    QTextStream stream(&output);
    stream << "Status: " << appFavoriteChangeStatusName(result.status) << '\n';
    stream << "App: " << formatAppId(result.appId) << '\n';
    stream << "Favorite: " << (result.favorite ? QStringLiteral("true") : QStringLiteral("false")) << '\n';
    return output;
}

QString appOpenStatusName(const AppOpenStatus status)
{
    switch (status) {
    case AppOpenStatus::Accepted:
        return QStringLiteral("accepted");
    case AppOpenStatus::AppNotFound:
        return QStringLiteral("app_not_found");
    case AppOpenStatus::LaunchFailed:
        return QStringLiteral("launch_failed");
    }

    return QStringLiteral("launch_failed");
}

QJsonObject toJsonObject(const AppOpenResult &result)
{
    QJsonObject json;
    json[QStringLiteral("status")] = appOpenStatusName(result.status);
    json[QStringLiteral("appId")] = result.appId;
    return json;
}

QString formatHumanReadableResult(const AppOpenResult &result)
{
    QString output;
    QTextStream stream(&output);
    stream << "Status: " << appOpenStatusName(result.status) << '\n';
    stream << "App: " << formatAppId(result.appId) << '\n';
    return output;
}

} // namespace plasma_bridge::control
