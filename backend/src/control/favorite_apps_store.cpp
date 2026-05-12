#include "control/favorite_apps_store.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

namespace plasma_bridge::control
{
namespace
{

constexpr qint64 kFavoriteAppsVersion = 1;

QStringList normalizedAppIds(const QStringList &appIds)
{
    QStringList normalized;
    for (const QString &appId : appIds) {
        if (appId.isEmpty() || normalized.contains(appId)) {
            continue;
        }
        normalized.append(appId);
    }

    return normalized;
}

FavoriteAppsStoreLoadResult loadError(const QString &message)
{
    FavoriteAppsStoreLoadResult result;
    result.status = FavoriteAppsStoreStatus::StorageError;
    result.errorMessage = message;
    return result;
}

FavoriteAppsStoreSaveResult saveError(const QString &message)
{
    FavoriteAppsStoreSaveResult result;
    result.status = FavoriteAppsStoreStatus::StorageError;
    result.errorMessage = message;
    return result;
}

} // namespace

FavoriteAppsStore::FavoriteAppsStore(QString filePath)
    : m_filePath(std::move(filePath))
{
}

QString FavoriteAppsStore::filePath() const
{
    return m_filePath;
}

FavoriteAppsStoreLoadResult FavoriteAppsStore::load() const
{
    QFile file(m_filePath);
    if (!file.exists()) {
        return {};
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return loadError(file.errorString());
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return loadError(parseError.errorString());
    }

    if (!document.isObject()) {
        return loadError(QStringLiteral("Favorites file must contain a JSON object."));
    }

    const QJsonObject object = document.object();
    const QJsonValue versionValue = object.value(QStringLiteral("version"));
    if (!versionValue.isDouble() || static_cast<qint64>(versionValue.toDouble()) != kFavoriteAppsVersion) {
        return loadError(QStringLiteral("Favorites file version is unsupported."));
    }

    const QJsonValue favoriteAppIdsValue = object.value(QStringLiteral("favoriteAppIds"));
    if (!favoriteAppIdsValue.isArray()) {
        return loadError(QStringLiteral("Favorites file must contain a favoriteAppIds array."));
    }

    QStringList appIds;
    const QJsonArray favoriteAppIds = favoriteAppIdsValue.toArray();
    appIds.reserve(favoriteAppIds.size());
    for (const QJsonValue &value : favoriteAppIds) {
        if (!value.isString()) {
            return loadError(QStringLiteral("Favorites file favoriteAppIds entries must be strings."));
        }
        appIds.append(value.toString());
    }

    FavoriteAppsStoreLoadResult result;
    result.appIds = normalizedAppIds(appIds);
    return result;
}

FavoriteAppsStoreSaveResult FavoriteAppsStore::save(const QStringList &appIds) const
{
    const QFileInfo fileInfo(m_filePath);
    const QString parentPath = fileInfo.absolutePath();
    if (!parentPath.isEmpty()) {
        QDir parentDirectory(parentPath);
        if (!parentDirectory.exists() && !QDir().mkpath(parentPath)) {
            return saveError(QStringLiteral("Failed to create favorites directory."));
        }
    }

    QJsonArray favoriteAppIds;
    for (const QString &appId : normalizedAppIds(appIds)) {
        favoriteAppIds.append(appId);
    }

    QJsonObject object;
    object[QStringLiteral("version")] = static_cast<int>(kFavoriteAppsVersion);
    object[QStringLiteral("favoriteAppIds")] = favoriteAppIds;

    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return saveError(file.errorString());
    }

    const QByteArray bytes = QJsonDocument(object).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size()) {
        return saveError(file.errorString());
    }

    if (!file.commit()) {
        return saveError(file.errorString());
    }

    return {};
}

} // namespace plasma_bridge::control
