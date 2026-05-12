#include "common/app_state.h"

namespace plasma_bridge
{

QJsonObject toJsonObject(const AppInfo &app)
{
    QJsonObject json;
    json[QStringLiteral("appId")] = app.appId;
    json[QStringLiteral("name")] = app.name;
    json[QStringLiteral("genericName")] = app.genericName;
    json[QStringLiteral("desktopEntryName")] = app.desktopEntryName;
    json[QStringLiteral("menuId")] = app.menuId;
    json[QStringLiteral("iconUrl")] = app.iconUrl;
    return json;
}

} // namespace plasma_bridge
