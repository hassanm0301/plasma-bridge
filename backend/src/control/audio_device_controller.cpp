#include "control/audio_device_controller.h"

#include <QJsonValue>
#include <QTextStream>

namespace plasma_bridge::control
{
namespace
{

QJsonValue boolOrNull(const std::optional<bool> &value)
{
    if (!value.has_value()) {
        return QJsonValue(QJsonValue::Null);
    }

    return QJsonValue(*value);
}

QString formatBoolValue(const std::optional<bool> &value)
{
    if (!value.has_value()) {
        return QStringLiteral("(none)");
    }

    return *value ? QStringLiteral("true") : QStringLiteral("false");
}

QString formatDeviceId(const QString &deviceId)
{
    return deviceId.isEmpty() ? QStringLiteral("(none)") : deviceId;
}

} // namespace

QString audioDeviceChangeStatusName(const AudioDeviceChangeStatus status)
{
    switch (status) {
    case AudioDeviceChangeStatus::Accepted:
        return QStringLiteral("accepted");
    case AudioDeviceChangeStatus::NotReady:
        return QStringLiteral("not_ready");
    case AudioDeviceChangeStatus::SinkNotFound:
        return QStringLiteral("sink_not_found");
    case AudioDeviceChangeStatus::SourceNotFound:
        return QStringLiteral("source_not_found");
    case AudioDeviceChangeStatus::SinkNotWritable:
        return QStringLiteral("sink_not_writable");
    case AudioDeviceChangeStatus::SourceNotWritable:
        return QStringLiteral("source_not_writable");
    }

    return QStringLiteral("not_ready");
}

QJsonObject toJsonObject(const DefaultDeviceChangeResult &result)
{
    QJsonObject json;
    json[QStringLiteral("status")] = audioDeviceChangeStatusName(result.status);
    json[QStringLiteral("deviceId")] = result.deviceId;
    return json;
}

QJsonObject toJsonObject(const MuteChangeResult &result)
{
    QJsonObject json;
    json[QStringLiteral("status")] = audioDeviceChangeStatusName(result.status);
    json[QStringLiteral("deviceId")] = result.deviceId;
    json[QStringLiteral("requestedMuted")] = boolOrNull(result.requestedMuted);
    json[QStringLiteral("targetMuted")] = boolOrNull(result.targetMuted);
    json[QStringLiteral("previousMuted")] = boolOrNull(result.previousMuted);
    return json;
}

QString formatHumanReadableResult(const DefaultDeviceChangeResult &result)
{
    QString output;
    QTextStream stream(&output);

    stream << "Status: " << audioDeviceChangeStatusName(result.status) << '\n';
    stream << "Device: " << formatDeviceId(result.deviceId) << '\n';

    return output;
}

QString formatHumanReadableResult(const MuteChangeResult &result)
{
    QString output;
    QTextStream stream(&output);

    stream << "Status: " << audioDeviceChangeStatusName(result.status) << '\n';
    stream << "Device: " << formatDeviceId(result.deviceId) << '\n';
    stream << "Requested Muted: " << formatBoolValue(result.requestedMuted) << '\n';
    stream << "Target Muted: " << formatBoolValue(result.targetMuted) << '\n';
    stream << "Previous Muted: " << formatBoolValue(result.previousMuted) << '\n';

    return output;
}

} // namespace plasma_bridge::control
