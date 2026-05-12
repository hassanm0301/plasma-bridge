#pragma once

#include "control/app_controller.h"

#include <QString>

namespace plasma_bridge::api
{

enum class AppControlAction {
    Favorite,
    Unfavorite,
    Open,
};

enum class AppControlRouteMatch {
    NoMatch,
    Match,
    InvalidAppId,
};

struct AppControlRoute {
    QString appId;
    AppControlAction action = AppControlAction::Open;
};

struct AppControlRouteParseResult {
    AppControlRouteMatch match = AppControlRouteMatch::NoMatch;
    AppControlRoute route;
};

AppControlRouteParseResult parseAppControlRoute(const QString &path);
bool parseAppOpenOptions(const QByteArray &target,
                         control::AppOpenOptions *outOptions,
                         QString *errorMessage = nullptr);
int httpStatusCodeForAppFavoriteChangeStatus(control::AppFavoriteChangeStatus status);
int httpStatusCodeForAppOpenStatus(control::AppOpenStatus status);

} // namespace plasma_bridge::api
