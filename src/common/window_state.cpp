#include "common/window_state.h"

#include <QJsonArray>
#include <QTextStream>

#include <algorithm>

namespace plasma_bridge
{
namespace
{

QJsonValue stringOrNull(const QString &value)
{
    return value.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(value);
}

QJsonValue numberOrNull(const quint32 value)
{
    return value == 0 ? QJsonValue(QJsonValue::Null) : QJsonValue(static_cast<qint64>(value));
}

QString boolText(const bool value)
{
    return value ? QStringLiteral("yes") : QStringLiteral("no");
}

QString joinValuesOrFallback(const QStringList &values, const QString &fallback)
{
    return values.isEmpty() ? fallback : values.join(QStringLiteral(", "));
}

void appendWindow(QTextStream &stream, const WindowState &window)
{
    stream << (window.isActive ? "* " : "  ")
           << (window.title.isEmpty() ? QStringLiteral("(untitled)") : window.title) << '\n';
    stream << "  id: " << window.id << '\n';
    stream << "  appId: " << (window.appId.isEmpty() ? QStringLiteral("(none)") : window.appId) << '\n';
    stream << "  pid: "
           << (window.pid == 0 ? QStringLiteral("(none)") : QString::number(window.pid)) << '\n';
    stream << "  minimized: " << boolText(window.isMinimized) << '\n';
    stream << "  maximized: " << boolText(window.isMaximized) << '\n';
    stream << "  fullscreen: " << boolText(window.isFullscreen) << '\n';
    stream << "  onAllDesktops: " << boolText(window.isOnAllDesktops) << '\n';
    stream << "  skipTaskbar: " << boolText(window.skipTaskbar) << '\n';
    stream << "  skipSwitcher: " << boolText(window.skipSwitcher) << '\n';
    stream << "  geometry: (" << window.geometry.x << ", " << window.geometry.y << ") "
           << window.geometry.width << 'x' << window.geometry.height << '\n';
    stream << "  clientGeometry: (" << window.clientGeometry.x << ", " << window.clientGeometry.y << ") "
           << window.clientGeometry.width << 'x' << window.clientGeometry.height << '\n';
    stream << "  virtualDesktopIds: "
           << joinValuesOrFallback(window.virtualDesktopIds, QStringLiteral("(all)")) << '\n';
    stream << "  activityIds: "
           << joinValuesOrFallback(window.activityIds, QStringLiteral("(all)")) << '\n';
    stream << "  parentId: " << (window.parentId.isEmpty() ? QStringLiteral("(none)") : window.parentId) << '\n';
    stream << "  resourceName: "
           << (window.resourceName.isEmpty() ? QStringLiteral("(none)") : window.resourceName) << '\n';
}

} // namespace

WindowGeometryState toWindowGeometryState(const QRect &rect)
{
    WindowGeometryState state;
    state.x = rect.x();
    state.y = rect.y();
    state.width = rect.width();
    state.height = rect.height();
    return state;
}

QRect toQRect(const WindowGeometryState &state)
{
    return QRect(state.x, state.y, state.width, state.height);
}

QJsonObject toJsonObject(const WindowGeometryState &geometry)
{
    QJsonObject json;
    json[QStringLiteral("x")] = geometry.x;
    json[QStringLiteral("y")] = geometry.y;
    json[QStringLiteral("width")] = geometry.width;
    json[QStringLiteral("height")] = geometry.height;
    return json;
}

QJsonObject toJsonObject(const WindowState &window)
{
    QJsonArray virtualDesktopIds;
    for (const QString &desktopId : window.virtualDesktopIds) {
        virtualDesktopIds.append(desktopId);
    }

    QJsonArray activityIds;
    for (const QString &activityId : window.activityIds) {
        activityIds.append(activityId);
    }

    QJsonObject json;
    json[QStringLiteral("id")] = window.id;
    json[QStringLiteral("title")] = window.title;
    json[QStringLiteral("appId")] = stringOrNull(window.appId);
    json[QStringLiteral("pid")] = numberOrNull(window.pid);
    json[QStringLiteral("isActive")] = window.isActive;
    json[QStringLiteral("isMinimized")] = window.isMinimized;
    json[QStringLiteral("isMaximized")] = window.isMaximized;
    json[QStringLiteral("isFullscreen")] = window.isFullscreen;
    json[QStringLiteral("isOnAllDesktops")] = window.isOnAllDesktops;
    json[QStringLiteral("skipTaskbar")] = window.skipTaskbar;
    json[QStringLiteral("skipSwitcher")] = window.skipSwitcher;
    json[QStringLiteral("geometry")] = toJsonObject(window.geometry);
    json[QStringLiteral("clientGeometry")] = toJsonObject(window.clientGeometry);
    json[QStringLiteral("virtualDesktopIds")] = virtualDesktopIds;
    json[QStringLiteral("activityIds")] = activityIds;
    json[QStringLiteral("parentId")] = stringOrNull(window.parentId);
    json[QStringLiteral("resourceName")] = stringOrNull(window.resourceName);
    return json;
}

QJsonObject toJsonObject(const WindowSnapshot &snapshot)
{
    QJsonArray windows;
    for (const WindowState &window : snapshot.windows) {
        windows.append(toJsonObject(window));
    }

    QJsonObject json;
    json[QStringLiteral("activeWindowId")] = stringOrNull(snapshot.activeWindowId);
    json[QStringLiteral("windows")] = windows;
    return json;
}

std::optional<WindowState> activeWindow(const WindowSnapshot &snapshot)
{
    if (!snapshot.activeWindowId.isEmpty()) {
        const auto it =
            std::find_if(snapshot.windows.cbegin(), snapshot.windows.cend(), [&snapshot](const WindowState &window) {
                return window.id == snapshot.activeWindowId;
            });
        if (it != snapshot.windows.cend()) {
            return *it;
        }
    }

    const auto activeIt = std::find_if(snapshot.windows.cbegin(),
                                       snapshot.windows.cend(),
                                       [](const WindowState &window) { return window.isActive; });
    if (activeIt != snapshot.windows.cend()) {
        return *activeIt;
    }

    return std::nullopt;
}

QString formatHumanReadableWindowList(const WindowSnapshot &snapshot)
{
    QString output;
    QTextStream stream(&output);

    stream << "Window Count: " << snapshot.windows.size() << '\n';
    stream << "Active Window: "
           << (snapshot.activeWindowId.isEmpty() ? QStringLiteral("(none)") : snapshot.activeWindowId) << '\n';

    for (const WindowState &window : snapshot.windows) {
        stream << '\n';
        appendWindow(stream, window);
    }

    return output;
}

QString formatHumanReadableActiveWindow(const std::optional<WindowState> &window)
{
    QString output;
    QTextStream stream(&output);

    if (!window.has_value()) {
        stream << "Active Window: (none)\n";
        return output;
    }

    stream << "Active Window:\n\n";
    appendWindow(stream, *window);
    return output;
}

} // namespace plasma_bridge
