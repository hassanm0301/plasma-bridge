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

class WindowActivationController
{
public:
    virtual ~WindowActivationController() = default;

    virtual WindowActivationResult activateWindow(const QString &windowId) = 0;
};

QString windowActivationStatusName(WindowActivationStatus status);
QJsonObject toJsonObject(const WindowActivationResult &result);
QString formatHumanReadableResult(const WindowActivationResult &result);

} // namespace plasma_bridge::control
