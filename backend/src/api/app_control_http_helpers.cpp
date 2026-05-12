#include "api/app_control_http_helpers.h"

#include <QUrl>
#include <QUrlQuery>

namespace plasma_bridge::api
{
namespace
{

const QString kControlAppsPrefix = QStringLiteral("/control/apps/");
const QString kFavoritePathSuffix = QStringLiteral("/favorite");
const QString kUnfavoritePathSuffix = QStringLiteral("/unfavorite");
const QString kOpenPathSuffix = QStringLiteral("/open");

} // namespace

AppControlRouteParseResult parseAppControlRoute(const QString &path)
{
    AppControlRouteParseResult result;
    if (!path.startsWith(kControlAppsPrefix)) {
        return result;
    }

    QString actionSuffix;
    AppControlAction action = AppControlAction::Open;
    if (path.endsWith(kFavoritePathSuffix)) {
        actionSuffix = kFavoritePathSuffix;
        action = AppControlAction::Favorite;
    } else if (path.endsWith(kUnfavoritePathSuffix)) {
        actionSuffix = kUnfavoritePathSuffix;
        action = AppControlAction::Unfavorite;
    } else if (path.endsWith(kOpenPathSuffix)) {
        actionSuffix = kOpenPathSuffix;
    } else {
        return result;
    }

    const qsizetype encodedAppIdLength = path.size() - kControlAppsPrefix.size() - actionSuffix.size();
    if (encodedAppIdLength <= 0) {
        return result;
    }

    const QString encodedAppId = path.mid(kControlAppsPrefix.size(), encodedAppIdLength);
    if (encodedAppId.contains(QLatin1Char('/'))) {
        result.match = AppControlRouteMatch::InvalidAppId;
        return result;
    }

    const QString appId = QUrl::fromPercentEncoding(encodedAppId.toUtf8());
    if (appId.isEmpty() || appId.contains(QLatin1Char('/'))) {
        result.match = AppControlRouteMatch::InvalidAppId;
        return result;
    }

    result.match = AppControlRouteMatch::Match;
    result.route.appId = appId;
    result.route.action = action;
    return result;
}

int httpStatusCodeForAppFavoriteChangeStatus(const control::AppFavoriteChangeStatus status)
{
    switch (status) {
    case control::AppFavoriteChangeStatus::Accepted:
        return 200;
    case control::AppFavoriteChangeStatus::AppNotFound:
        return 404;
    case control::AppFavoriteChangeStatus::StorageError:
        return 500;
    }

    return 500;
}

bool parseAppOpenOptions(const QByteArray &target,
                         control::AppOpenOptions *outOptions,
                         QString *errorMessage)
{
    if (outOptions == nullptr) {
        return false;
    }

    *outOptions = control::AppOpenOptions{};

    const QString requestTarget = QString::fromUtf8(target);
    const qsizetype queryIndex = requestTarget.indexOf(QLatin1Char('?'));
    if (queryIndex < 0) {
        return true;
    }

    QUrlQuery query;
    query.setQuery(requestTarget.mid(queryIndex + 1));

    if (!query.hasQueryItem(QStringLiteral("switchToExisting"))) {
        return true;
    }

    const QString value = query.queryItemValue(QStringLiteral("switchToExisting")).trimmed().toLower();
    if (value == QStringLiteral("true")) {
        outOptions->switchToExisting = true;
        return true;
    }
    if (value == QStringLiteral("false")) {
        outOptions->switchToExisting = false;
        return true;
    }

    if (errorMessage != nullptr) {
        *errorMessage = QStringLiteral("switchToExisting must be true or false.");
    }
    return false;
}

int httpStatusCodeForAppOpenStatus(const control::AppOpenStatus status)
{
    switch (status) {
    case control::AppOpenStatus::Accepted:
        return 200;
    case control::AppOpenStatus::AppNotFound:
        return 404;
    case control::AppOpenStatus::LaunchFailed:
        return 500;
    }

    return 500;
}

} // namespace plasma_bridge::api
