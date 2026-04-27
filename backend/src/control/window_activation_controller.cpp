#include "control/window_activation_controller.h"

#include <QTextStream>

namespace plasma_bridge::control
{
namespace
{

QString formatWindowId(const QString &windowId)
{
    return windowId.isEmpty() ? QStringLiteral("(none)") : windowId;
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
    QJsonObject json;
    json[QStringLiteral("status")] = windowActivationStatusName(result.status);
    json[QStringLiteral("windowId")] = result.windowId;
    return json;
}

QString formatHumanReadableResult(const WindowActivationResult &result)
{
    QString output;
    QTextStream stream(&output);

    stream << "Status: " << windowActivationStatusName(result.status) << '\n';
    stream << "Window: " << formatWindowId(result.windowId) << '\n';

    return output;
}

} // namespace plasma_bridge::control
