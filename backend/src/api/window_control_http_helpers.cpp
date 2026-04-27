#include "api/window_control_http_helpers.h"

#include <QUrl>

namespace plasma_bridge::api
{
namespace
{

const QString kControlWindowsPrefix = QStringLiteral("/control/windows/");
const QString kActivePathSuffix = QStringLiteral("/active");

} // namespace

WindowControlRouteParseResult parseWindowControlRoute(const QString &path)
{
    WindowControlRouteParseResult result;
    if (!path.startsWith(kControlWindowsPrefix) || !path.endsWith(kActivePathSuffix)) {
        return result;
    }

    const qsizetype encodedWindowIdLength = path.size() - kControlWindowsPrefix.size() - kActivePathSuffix.size();
    if (encodedWindowIdLength <= 0) {
        return result;
    }

    const QString encodedWindowId = path.mid(kControlWindowsPrefix.size(), encodedWindowIdLength);
    if (encodedWindowId.contains(QLatin1Char('/'))) {
        result.match = WindowControlRouteMatch::InvalidWindowId;
        return result;
    }

    const QString windowId = QUrl::fromPercentEncoding(encodedWindowId.toUtf8());
    if (windowId.isEmpty() || windowId.contains(QLatin1Char('/'))) {
        result.match = WindowControlRouteMatch::InvalidWindowId;
        return result;
    }

    result.match = WindowControlRouteMatch::Match;
    result.route.windowId = windowId;
    return result;
}

int httpStatusCodeForWindowActivationStatus(const control::WindowActivationStatus status)
{
    switch (status) {
    case control::WindowActivationStatus::Accepted:
        return 200;
    case control::WindowActivationStatus::WindowNotFound:
        return 404;
    case control::WindowActivationStatus::WindowNotActivatable:
        return 409;
    case control::WindowActivationStatus::NotReady:
        return 503;
    }

    return 503;
}

} // namespace plasma_bridge::api
