#include "control/audio_volume_controller.h"

#include <QJsonValue>
#include <QTextStream>

#include <cmath>

namespace plasma_bridge::control
{
namespace
{

QJsonValue numberOrNull(const std::optional<double> &value)
{
    if (!value.has_value() || !std::isfinite(*value)) {
        return QJsonValue(QJsonValue::Null);
    }

    return QJsonValue(*value);
}

QString formatNormalizedValue(const std::optional<double> &value)
{
    if (!value.has_value()) {
        return QStringLiteral("(none)");
    }

    if (!std::isfinite(*value)) {
        return QStringLiteral("(invalid)");
    }

    return QStringLiteral("%1 (%2%)")
        .arg(QString::number(*value, 'f', 4),
             QString::number(*value * 100.0, 'f', 1));
}

} // namespace

QString volumeChangeStatusName(const VolumeChangeStatus status)
{
    switch (status) {
    case VolumeChangeStatus::Accepted:
        return QStringLiteral("accepted");
    case VolumeChangeStatus::NotReady:
        return QStringLiteral("not_ready");
    case VolumeChangeStatus::SinkNotFound:
        return QStringLiteral("sink_not_found");
    case VolumeChangeStatus::SinkNotWritable:
        return QStringLiteral("sink_not_writable");
    case VolumeChangeStatus::InvalidValue:
        return QStringLiteral("invalid_value");
    }

    return QStringLiteral("invalid_value");
}

QJsonObject toJsonObject(const VolumeChangeResult &result)
{
    QJsonObject json;
    json[QStringLiteral("status")] = volumeChangeStatusName(result.status);
    json[QStringLiteral("sinkId")] = result.sinkId;
    json[QStringLiteral("requestedValue")] = numberOrNull(result.requestedValue);
    json[QStringLiteral("targetValue")] = numberOrNull(result.targetValue);
    json[QStringLiteral("previousValue")] = numberOrNull(result.previousValue);
    return json;
}

QString formatHumanReadableResult(const VolumeChangeResult &result)
{
    QString output;
    QTextStream stream(&output);

    stream << "Status: " << volumeChangeStatusName(result.status) << '\n';
    stream << "Sink: " << (result.sinkId.isEmpty() ? QStringLiteral("(none)") : result.sinkId) << '\n';
    stream << "Requested Value: " << formatNormalizedValue(result.requestedValue) << '\n';
    stream << "Target Value: " << formatNormalizedValue(result.targetValue) << '\n';
    stream << "Previous Value: " << formatNormalizedValue(result.previousValue) << '\n';

    return output;
}

} // namespace plasma_bridge::control
