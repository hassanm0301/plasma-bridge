#pragma once

#include "common/app_state.h"

#include <QJsonObject>
#include <QList>
#include <QString>

#include <optional>

namespace plasma_bridge::control
{

enum class FavoriteAppsStatus {
    Accepted,
    StorageError,
};

struct FavoriteAppsResult {
    FavoriteAppsStatus status = FavoriteAppsStatus::Accepted;
    QList<plasma_bridge::AppInfo> apps;
    QString errorMessage;
};

enum class AppFavoriteChangeStatus {
    Accepted,
    AppNotFound,
    StorageError,
};

struct AppFavoriteChangeResult {
    AppFavoriteChangeStatus status = AppFavoriteChangeStatus::Accepted;
    QString appId;
    bool favorite = false;
    QString errorMessage;
};

enum class AppOpenStatus {
    Accepted,
    AppNotFound,
    LaunchFailed,
};

struct AppOpenResult {
    AppOpenStatus status = AppOpenStatus::Accepted;
    QString appId;
    QString errorMessage;
};

struct AppOpenOptions {
    bool switchToExisting = false;
};

class AppController
{
public:
    virtual ~AppController() = default;

    virtual QList<plasma_bridge::AppInfo> availableApps(const QString &query = {}) = 0;
    virtual std::optional<plasma_bridge::AppInfo> findApp(const QString &appId) = 0;
    virtual FavoriteAppsResult favoriteApps() = 0;
    virtual AppFavoriteChangeResult addFavorite(const QString &appId) = 0;
    virtual AppFavoriteChangeResult removeFavorite(const QString &appId) = 0;
    virtual AppOpenResult openApp(const QString &appId, const AppOpenOptions &options = {}) = 0;
};

QString favoriteAppsStatusName(FavoriteAppsStatus status);
QString appFavoriteChangeStatusName(AppFavoriteChangeStatus status);
QJsonObject toJsonObject(const AppFavoriteChangeResult &result);
QString formatHumanReadableResult(const AppFavoriteChangeResult &result);

QString appOpenStatusName(AppOpenStatus status);
QJsonObject toJsonObject(const AppOpenResult &result);
QString formatHumanReadableResult(const AppOpenResult &result);

} // namespace plasma_bridge::control
