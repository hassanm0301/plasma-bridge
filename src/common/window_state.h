#pragma once

#include <QJsonObject>
#include <QList>
#include <QRect>
#include <QString>
#include <QStringList>

#include <optional>

namespace plasma_bridge
{

struct WindowGeometryState {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct WindowState {
    QString id;
    QString title;
    QString appId;
    quint32 pid = 0;
    bool isActive = false;
    bool isMinimized = false;
    bool isMaximized = false;
    bool isFullscreen = false;
    bool isOnAllDesktops = false;
    bool skipTaskbar = false;
    bool skipSwitcher = false;
    WindowGeometryState geometry;
    WindowGeometryState clientGeometry;
    QStringList virtualDesktopIds;
    QStringList activityIds;
    QString parentId;
    QString resourceName;
};

struct WindowSnapshot {
    QList<WindowState> windows;
    QString activeWindowId;
};

WindowGeometryState toWindowGeometryState(const QRect &rect);
QRect toQRect(const WindowGeometryState &state);

QJsonObject toJsonObject(const WindowGeometryState &geometry);
QJsonObject toJsonObject(const WindowState &window);
QJsonObject toJsonObject(const WindowSnapshot &snapshot);

std::optional<WindowState> activeWindow(const WindowSnapshot &snapshot);

QString formatHumanReadableWindowList(const WindowSnapshot &snapshot);
QString formatHumanReadableActiveWindow(const std::optional<WindowState> &window);

} // namespace plasma_bridge
