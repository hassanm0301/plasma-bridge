#pragma once

#include "control/app_controller.h"
#include "control/favorite_apps_store.h"

namespace plasma_bridge::apps
{

class KdeAppController final : public control::AppController
{
public:
    explicit KdeAppController(QString favoritesFilePath);

    QList<plasma_bridge::AppInfo> availableApps(const QString &query = {}) override;
    std::optional<plasma_bridge::AppInfo> findApp(const QString &appId) override;
    control::FavoriteAppsResult favoriteApps() override;
    control::AppFavoriteChangeResult addFavorite(const QString &appId) override;
    control::AppFavoriteChangeResult removeFavorite(const QString &appId) override;
    control::AppOpenResult openApp(const QString &appId, const control::AppOpenOptions &options = {}) override;

private:
    control::FavoriteAppsStore m_favoriteAppsStore;
};

} // namespace plasma_bridge::apps
