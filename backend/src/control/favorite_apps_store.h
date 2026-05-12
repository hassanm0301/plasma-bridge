#pragma once

#include <QString>
#include <QStringList>

namespace plasma_bridge::control
{

enum class FavoriteAppsStoreStatus {
    Accepted,
    StorageError,
};

struct FavoriteAppsStoreLoadResult {
    FavoriteAppsStoreStatus status = FavoriteAppsStoreStatus::Accepted;
    QStringList appIds;
    QString errorMessage;
};

struct FavoriteAppsStoreSaveResult {
    FavoriteAppsStoreStatus status = FavoriteAppsStoreStatus::Accepted;
    QString errorMessage;
};

class FavoriteAppsStore
{
public:
    explicit FavoriteAppsStore(QString filePath);

    QString filePath() const;

    FavoriteAppsStoreLoadResult load() const;
    FavoriteAppsStoreSaveResult save(const QStringList &appIds) const;

private:
    QString m_filePath;
};

} // namespace plasma_bridge::control
