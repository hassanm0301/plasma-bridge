#include "adapters/apps/kde_app_controller.h"

#include <KApplicationTrader>
#include <KIO/ApplicationLauncherJob>
#include <KJob>
#include <KService>

#include <QEventLoop>
#include <QHash>
#include <QPointer>
#include <QUrl>

#include <algorithm>

namespace plasma_bridge::apps
{
namespace
{

QString appIconUrl(const QString &iconName)
{
    if (iconName.isEmpty()) {
        return {};
    }

    return QStringLiteral("/icons/apps/%1").arg(QString::fromUtf8(QUrl::toPercentEncoding(iconName)));
}

bool isResolvableApplication(const KService::Ptr &service)
{
    return service && service->isApplication() && !service->storageId().isEmpty();
}

bool isAvailableApplication(const KService::Ptr &service)
{
    return isResolvableApplication(service) && !service->noDisplay() && service->showInCurrentDesktop();
}

plasma_bridge::AppInfo appInfoFromService(const KService::Ptr &service)
{
    plasma_bridge::AppInfo app;
    app.appId = service->storageId();
    app.name = service->name();
    app.genericName = service->genericName();
    app.desktopEntryName = service->desktopEntryName();
    app.menuId = service->menuId();
    app.iconUrl = appIconUrl(service->icon());
    return app;
}

QList<plasma_bridge::AppInfo> sortAndDeduplicateApps(const KService::List &services)
{
    QHash<QString, plasma_bridge::AppInfo> appsById;
    for (const KService::Ptr &service : services) {
        if (!isResolvableApplication(service)) {
            continue;
        }
        const plasma_bridge::AppInfo app = appInfoFromService(service);
        if (!appsById.contains(app.appId)) {
            appsById.insert(app.appId, app);
        }
    }

    QList<plasma_bridge::AppInfo> apps = appsById.values();
    std::sort(apps.begin(), apps.end(), [](const plasma_bridge::AppInfo &left, const plasma_bridge::AppInfo &right) {
        const int nameCompare = QString::localeAwareCompare(left.name, right.name);
        if (nameCompare != 0) {
            return nameCompare < 0;
        }
        return left.appId < right.appId;
    });
    return apps;
}

QString normalizedQuery(const QString &query)
{
    return query.trimmed();
}

bool appMatchesQuery(const plasma_bridge::AppInfo &app, const QString &query)
{
    if (query.isEmpty()) {
        return true;
    }

    return app.name.contains(query, Qt::CaseInsensitive) || app.genericName.contains(query, Qt::CaseInsensitive)
        || app.desktopEntryName.contains(query, Qt::CaseInsensitive) || app.appId.contains(query, Qt::CaseInsensitive);
}

KService::Ptr resolveApplication(const QString &appId)
{
    const KService::Ptr service = KService::serviceByStorageId(appId);
    return isResolvableApplication(service) ? service : KService::Ptr();
}

} // namespace

KdeAppController::KdeAppController(QString favoritesFilePath)
    : m_favoriteAppsStore(std::move(favoritesFilePath))
{
}

QList<plasma_bridge::AppInfo> KdeAppController::availableApps(const QString &query)
{
    const KService::List services = KApplicationTrader::query([](const KService::Ptr &service) {
        return isAvailableApplication(service);
    });
    QList<plasma_bridge::AppInfo> apps = sortAndDeduplicateApps(services);
    const QString trimmedQuery = normalizedQuery(query);
    if (trimmedQuery.isEmpty()) {
        return apps;
    }

    apps.erase(std::remove_if(apps.begin(), apps.end(), [&trimmedQuery](const plasma_bridge::AppInfo &app) {
        return !appMatchesQuery(app, trimmedQuery);
    }),
               apps.end());
    return apps;
}

std::optional<plasma_bridge::AppInfo> KdeAppController::findApp(const QString &appId)
{
    const KService::Ptr service = resolveApplication(appId);
    if (!service) {
        return std::nullopt;
    }

    return appInfoFromService(service);
}

control::FavoriteAppsResult KdeAppController::favoriteApps()
{
    const control::FavoriteAppsStoreLoadResult loadResult = m_favoriteAppsStore.load();
    if (loadResult.status != control::FavoriteAppsStoreStatus::Accepted) {
        control::FavoriteAppsResult result;
        result.status = control::FavoriteAppsStatus::StorageError;
        result.errorMessage = loadResult.errorMessage;
        return result;
    }

    control::FavoriteAppsResult result;
    for (const QString &appId : loadResult.appIds) {
        const KService::Ptr service = resolveApplication(appId);
        if (!service) {
            continue;
        }
        result.apps.append(appInfoFromService(service));
    }
    return result;
}

control::AppFavoriteChangeResult KdeAppController::addFavorite(const QString &appId)
{
    control::AppFavoriteChangeResult result;
    result.appId = appId;
    result.favorite = true;

    if (!resolveApplication(appId)) {
        result.status = control::AppFavoriteChangeStatus::AppNotFound;
        return result;
    }

    const control::FavoriteAppsStoreLoadResult loadResult = m_favoriteAppsStore.load();
    if (loadResult.status != control::FavoriteAppsStoreStatus::Accepted) {
        result.status = control::AppFavoriteChangeStatus::StorageError;
        result.errorMessage = loadResult.errorMessage;
        return result;
    }

    QStringList appIds = loadResult.appIds;
    if (!appIds.contains(appId)) {
        appIds.append(appId);
    }

    const control::FavoriteAppsStoreSaveResult saveResult = m_favoriteAppsStore.save(appIds);
    if (saveResult.status != control::FavoriteAppsStoreStatus::Accepted) {
        result.status = control::AppFavoriteChangeStatus::StorageError;
        result.errorMessage = saveResult.errorMessage;
        return result;
    }

    result.status = control::AppFavoriteChangeStatus::Accepted;
    return result;
}

control::AppFavoriteChangeResult KdeAppController::removeFavorite(const QString &appId)
{
    control::AppFavoriteChangeResult result;
    result.appId = appId;
    result.favorite = false;

    const control::FavoriteAppsStoreLoadResult loadResult = m_favoriteAppsStore.load();
    if (loadResult.status != control::FavoriteAppsStoreStatus::Accepted) {
        result.status = control::AppFavoriteChangeStatus::StorageError;
        result.errorMessage = loadResult.errorMessage;
        return result;
    }

    QStringList appIds = loadResult.appIds;
    appIds.removeAll(appId);

    const control::FavoriteAppsStoreSaveResult saveResult = m_favoriteAppsStore.save(appIds);
    if (saveResult.status != control::FavoriteAppsStoreStatus::Accepted) {
        result.status = control::AppFavoriteChangeStatus::StorageError;
        result.errorMessage = saveResult.errorMessage;
        return result;
    }

    result.status = control::AppFavoriteChangeStatus::Accepted;
    return result;
}

control::AppOpenResult KdeAppController::openApp(const QString &appId, const control::AppOpenOptions &options)
{
    Q_UNUSED(options);

    control::AppOpenResult result;
    result.appId = appId;

    const KService::Ptr service = resolveApplication(appId);
    if (!service) {
        result.status = control::AppOpenStatus::AppNotFound;
        return result;
    }

    auto *job = new KIO::ApplicationLauncherJob(service);
    job->setAutoDelete(false);

    QEventLoop loop;
    QObject::connect(job, &KJob::result, &loop, &QEventLoop::quit);

    job->start();
    loop.exec();

    if (job->error() != 0) {
        result.status = control::AppOpenStatus::LaunchFailed;
        result.errorMessage = job->errorText();
        delete job;
        return result;
    }

    delete job;
    result.status = control::AppOpenStatus::Accepted;
    return result;
}

} // namespace plasma_bridge::apps
