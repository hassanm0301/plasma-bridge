#pragma once

#include <QJsonObject>
#include <QString>

namespace plasma_bridge::control
{

enum class WindowActivationStatus {
    Accepted,
    NotReady,
    WindowNotFound,
    WindowNotActivatable,
};

struct WindowActivationResult {
    WindowActivationStatus status = WindowActivationStatus::NotReady;
    QString windowId;
};

enum class WindowCloseStatus {
    Accepted,
    NotReady,
    WindowNotFound,
    WindowNotCloseable,
};

struct WindowCloseResult {
    WindowCloseStatus status = WindowCloseStatus::NotReady;
    QString windowId;
};

class WindowControlController
{
public:
    virtual ~WindowControlController() = default;

    virtual WindowActivationResult activateWindow(const QString &windowId) = 0;
    virtual WindowCloseResult closeWindow(const QString &windowId) = 0;
};

QString windowActivationStatusName(WindowActivationStatus status);
QJsonObject toJsonObject(const WindowActivationResult &result);
QString formatHumanReadableResult(const WindowActivationResult &result);

QString windowCloseStatusName(WindowCloseStatus status);
QJsonObject toJsonObject(const WindowCloseResult &result);
QString formatHumanReadableResult(const WindowCloseResult &result);

} // namespace plasma_bridge::control
