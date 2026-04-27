#pragma once

#include "control/window_activation_controller.h"

#include <QString>

namespace plasma_bridge::api
{

enum class WindowControlRouteMatch {
    NoMatch,
    Match,
    InvalidWindowId,
};

struct WindowControlRoute {
    QString windowId;
};

struct WindowControlRouteParseResult {
    WindowControlRouteMatch match = WindowControlRouteMatch::NoMatch;
    WindowControlRoute route;
};

WindowControlRouteParseResult parseWindowControlRoute(const QString &path);
int httpStatusCodeForWindowActivationStatus(control::WindowActivationStatus status);

} // namespace plasma_bridge::api
