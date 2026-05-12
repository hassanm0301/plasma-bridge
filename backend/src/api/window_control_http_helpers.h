#pragma once

#include "control/window_control_controller.h"

#include <QString>

namespace plasma_bridge::api
{

enum class WindowControlAction {
    Activate,
    Close,
};

enum class WindowControlRouteMatch {
    NoMatch,
    Match,
    InvalidWindowId,
};

struct WindowControlRoute {
    QString windowId;
    WindowControlAction action = WindowControlAction::Activate;
};

struct WindowControlRouteParseResult {
    WindowControlRouteMatch match = WindowControlRouteMatch::NoMatch;
    WindowControlRoute route;
};

WindowControlRouteParseResult parseWindowControlRoute(const QString &path);
int httpStatusCodeForWindowActivationStatus(control::WindowActivationStatus status);
int httpStatusCodeForWindowCloseStatus(control::WindowCloseStatus status);

} // namespace plasma_bridge::api
