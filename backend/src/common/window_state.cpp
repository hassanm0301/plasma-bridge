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

QStringList stringListFromJson(const QJsonValue &value)
{
    QStringList values;
    if (!value.isArray()) {
        return values;
    }

    const QJsonArray array = value.toArray();
    values.reserve(array.size());
    for (const QJsonValue &entry : array) {
        if (entry.isString()) {
            values.append(entry.toString());
        }
    }

    return values;
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
    stream << "  iconUrl: " << (window.iconUrl.isEmpty() ? QStringLiteral("(none)") : window.iconUrl) << '\n';
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
    json[QStringLiteral("iconUrl")] = stringOrNull(window.iconUrl);
    return json;
}

QJsonObject toJsonObject(const WindowSnapshot &snapshot)
{
    const WindowSnapshot normalizedSnapshot = normalizeWindowSnapshot(snapshot);
    QJsonArray windows;
    for (const WindowState &window : normalizedSnapshot.windows) {
        windows.append(toJsonObject(window));
    }

    QJsonObject json;
    const std::optional<WindowState> active = activeWindow(normalizedSnapshot);
    json[QStringLiteral("activeWindowId")] = stringOrNull(normalizedSnapshot.activeWindowId);
    json[QStringLiteral("activeWindow")] =
        active.has_value() ? QJsonValue(toJsonObject(*active)) : QJsonValue(QJsonValue::Null);
    json[QStringLiteral("windows")] = windows;
    return json;
}

std::optional<WindowGeometryState> windowGeometryStateFromJson(const QJsonObject &json)
{
    const QJsonValue xValue = json.value(QStringLiteral("x"));
    const QJsonValue yValue = json.value(QStringLiteral("y"));
    const QJsonValue widthValue = json.value(QStringLiteral("width"));
    const QJsonValue heightValue = json.value(QStringLiteral("height"));
    if (!xValue.isDouble() || !yValue.isDouble() || !widthValue.isDouble() || !heightValue.isDouble()) {
        return std::nullopt;
    }

    WindowGeometryState geometry;
    geometry.x = xValue.toInt();
    geometry.y = yValue.toInt();
    geometry.width = widthValue.toInt();
    geometry.height = heightValue.toInt();
    return geometry;
}

std::optional<WindowState> windowStateFromJson(const QJsonObject &json)
{
    const QString id = json.value(QStringLiteral("id")).toString();
    if (id.isEmpty()) {
        return std::nullopt;
    }

    const QJsonValue geometryValue = json.value(QStringLiteral("geometry"));
    const QJsonValue clientGeometryValue = json.value(QStringLiteral("clientGeometry"));
    if (!geometryValue.isObject() || !clientGeometryValue.isObject()) {
        return std::nullopt;
    }

    const std::optional<WindowGeometryState> geometry = windowGeometryStateFromJson(geometryValue.toObject());
    const std::optional<WindowGeometryState> clientGeometry =
        windowGeometryStateFromJson(clientGeometryValue.toObject());
    if (!geometry.has_value() || !clientGeometry.has_value()) {
        return std::nullopt;
    }

    WindowState window;
    window.id = id;
    window.title = json.value(QStringLiteral("title")).toString();
    if (const QJsonValue appIdValue = json.value(QStringLiteral("appId")); appIdValue.isString()) {
        window.appId = appIdValue.toString();
    }
    if (const QJsonValue pidValue = json.value(QStringLiteral("pid")); pidValue.isDouble()) {
        window.pid = static_cast<quint32>(pidValue.toInteger());
    }
    window.isActive = json.value(QStringLiteral("isActive")).toBool(false);
    window.isMinimized = json.value(QStringLiteral("isMinimized")).toBool(false);
    window.isMaximized = json.value(QStringLiteral("isMaximized")).toBool(false);
    window.isFullscreen = json.value(QStringLiteral("isFullscreen")).toBool(false);
    window.isOnAllDesktops = json.value(QStringLiteral("isOnAllDesktops")).toBool(false);
    window.skipTaskbar = json.value(QStringLiteral("skipTaskbar")).toBool(false);
    window.skipSwitcher = json.value(QStringLiteral("skipSwitcher")).toBool(false);
    window.geometry = *geometry;
    window.clientGeometry = *clientGeometry;
    window.virtualDesktopIds = stringListFromJson(json.value(QStringLiteral("virtualDesktopIds")));
    window.activityIds = stringListFromJson(json.value(QStringLiteral("activityIds")));
    if (const QJsonValue parentIdValue = json.value(QStringLiteral("parentId")); parentIdValue.isString()) {
        window.parentId = parentIdValue.toString();
    }
    if (const QJsonValue resourceNameValue = json.value(QStringLiteral("resourceName")); resourceNameValue.isString()) {
        window.resourceName = resourceNameValue.toString();
    }
    if (const QJsonValue iconUrlValue = json.value(QStringLiteral("iconUrl")); iconUrlValue.isString()) {
        window.iconUrl = iconUrlValue.toString();
    }

    return window;
}

std::optional<WindowSnapshot> windowSnapshotFromJson(const QJsonObject &json)
{
    const QJsonValue windowsValue = json.value(QStringLiteral("windows"));
    if (!windowsValue.isArray()) {
        return std::nullopt;
    }

    WindowSnapshot snapshot;
    if (const QJsonValue activeWindowIdValue = json.value(QStringLiteral("activeWindowId")); activeWindowIdValue.isString()) {
        snapshot.activeWindowId = activeWindowIdValue.toString();
    }

    const QJsonArray windowsArray = windowsValue.toArray();
    snapshot.windows.reserve(windowsArray.size());
    for (const QJsonValue &windowValue : windowsArray) {
        if (!windowValue.isObject()) {
            return std::nullopt;
        }

        const std::optional<WindowState> window = windowStateFromJson(windowValue.toObject());
        if (!window.has_value()) {
            return std::nullopt;
        }

        snapshot.windows.append(*window);
    }

    return normalizeWindowSnapshot(snapshot);
}

WindowSnapshot normalizeWindowSnapshot(const WindowSnapshot &snapshot)
{
    WindowSnapshot normalized = snapshot;
    const std::optional<WindowState> active = activeWindow(snapshot);

    if (!active.has_value()) {
        normalized.activeWindowId.clear();
        for (WindowState &window : normalized.windows) {
            window.isActive = false;
        }
        return normalized;
    }

    normalized.activeWindowId = active->id;
    for (WindowState &window : normalized.windows) {
        window.isActive = window.id == normalized.activeWindowId;
    }

    return normalized;
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
