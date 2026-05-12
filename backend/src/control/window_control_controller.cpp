#include "control/window_control_controller.h"

#include <QTextStream>

namespace plasma_bridge::control
{
namespace
{

QString formatWindowId(const QString &windowId)
{
    return windowId.isEmpty() ? QStringLiteral("(none)") : windowId;
}

QJsonObject buildWindowPayload(const QString &statusName, const QString &windowId)
{
    QJsonObject json;
    json[QStringLiteral("status")] = statusName;
    json[QStringLiteral("windowId")] = windowId;
    return json;
}

QString formatWindowResultText(const QString &statusName, const QString &windowId)
{
    QString output;
    QTextStream stream(&output);

    stream << "Status: " << statusName << '\n';
    stream << "Window: " << formatWindowId(windowId) << '\n';

    return output;
}

} // namespace

QString windowActivationStatusName(const WindowActivationStatus status)
{
    switch (status) {
    case WindowActivationStatus::Accepted:
        return QStringLiteral("accepted");
    case WindowActivationStatus::NotReady:
        return QStringLiteral("not_ready");
    case WindowActivationStatus::WindowNotFound:
        return QStringLiteral("window_not_found");
    case WindowActivationStatus::WindowNotActivatable:
        return QStringLiteral("window_not_activatable");
    }

    return QStringLiteral("not_ready");
}

QJsonObject toJsonObject(const WindowActivationResult &result)
{
    return buildWindowPayload(windowActivationStatusName(result.status), result.windowId);
}

QString formatHumanReadableResult(const WindowActivationResult &result)
{
    return formatWindowResultText(windowActivationStatusName(result.status), result.windowId);
}

QString windowCloseStatusName(const WindowCloseStatus status)
{
    switch (status) {
    case WindowCloseStatus::Accepted:
        return QStringLiteral("accepted");
    case WindowCloseStatus::NotReady:
        return QStringLiteral("not_ready");
    case WindowCloseStatus::WindowNotFound:
        return QStringLiteral("window_not_found");
    case WindowCloseStatus::WindowNotCloseable:
        return QStringLiteral("window_not_closeable");
    }

    return QStringLiteral("not_ready");
}

QJsonObject toJsonObject(const WindowCloseResult &result)
{
    return buildWindowPayload(windowCloseStatusName(result.status), result.windowId);
}

QString formatHumanReadableResult(const WindowCloseResult &result)
{
    return formatWindowResultText(windowCloseStatusName(result.status), result.windowId);
}

} // namespace plasma_bridge::control
