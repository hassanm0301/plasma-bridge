#pragma once

#include <QJsonObject>
#include <QString>

namespace plasma_bridge
{

struct AppInfo {
    QString appId;
    QString name;
    QString genericName;
    QString desktopEntryName;
    QString menuId;
    QString iconUrl;
};

QJsonObject toJsonObject(const AppInfo &app);

} // namespace plasma_bridge
