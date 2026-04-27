#include "api/json_envelope.h"

#include "control/audio_device_controller.h"
#include "control/audio_volume_controller.h"
#include "control/window_activation_controller.h"

#include <cmath>

namespace plasma_bridge::api
{
namespace
{

QJsonObject buildEnvelopeObject(const QJsonValue &payload, const QJsonValue &error)
{
    QJsonObject envelope;
    envelope[QStringLiteral("payload")] = payload;
    envelope[QStringLiteral("error")] = error;
    return envelope;
}

QJsonValue boolOrNull(const std::optional<bool> &value)
{
    if (!value.has_value()) {
        return QJsonValue(QJsonValue::Null);
    }

    return QJsonValue(*value);
}

QJsonValue numberOrNull(const std::optional<double> &value)
{
    if (!value.has_value() || !std::isfinite(*value)) {
        return QJsonValue(QJsonValue::Null);
    }

    return QJsonValue(*value);
}

} // namespace

QJsonObject buildApiError(const QString &code, const QString &message, const QJsonObject &details)
{
    QJsonObject error;
    error[QStringLiteral("code")] = code;
    error[QStringLiteral("message")] = message;
    if (!details.isEmpty()) {
        error[QStringLiteral("details")] = details;
    }

    return error;
}

QJsonDocument buildHttpSuccessEnvelope(const QJsonValue &payload)
{
    return QJsonDocument(buildEnvelopeObject(payload, QJsonValue(QJsonValue::Null)));
}

QJsonDocument buildHttpErrorEnvelope(const QString &code, const QString &message, const QJsonObject &details)
{
    return QJsonDocument(buildEnvelopeObject(QJsonValue(QJsonValue::Null), buildApiError(code, message, details)));
}

QJsonObject buildWebSocketSuccessEnvelope(const QString &type, const QJsonValue &payload)
{
    QJsonObject envelope = buildEnvelopeObject(payload, QJsonValue(QJsonValue::Null));
    envelope[QStringLiteral("type")] = type;
    return envelope;
}

QJsonObject buildWebSocketErrorEnvelope(const QString &code, const QString &message, const QJsonObject &details)
{
    QJsonObject envelope = buildEnvelopeObject(QJsonValue(QJsonValue::Null), buildApiError(code, message, details));
    envelope[QStringLiteral("type")] = QStringLiteral("error");
    return envelope;
}

QJsonObject buildDefaultDevicePayload(const control::DefaultDeviceChangeResult &result)
{
    QJsonObject payload;
    payload[QStringLiteral("deviceId")] = result.deviceId;
    return payload;
}

QJsonObject buildDefaultDeviceErrorDetails(const control::DefaultDeviceChangeResult &result)
{
    QJsonObject details;
    if (!result.deviceId.isEmpty()) {
        details[QStringLiteral("deviceId")] = result.deviceId;
    }

    return details;
}

QJsonObject buildMutePayload(const control::MuteChangeResult &result)
{
    QJsonObject payload;
    payload[QStringLiteral("deviceId")] = result.deviceId;
    payload[QStringLiteral("requestedMuted")] = boolOrNull(result.requestedMuted);
    payload[QStringLiteral("targetMuted")] = boolOrNull(result.targetMuted);
    payload[QStringLiteral("previousMuted")] = boolOrNull(result.previousMuted);
    return payload;
}

QJsonObject buildMuteErrorDetails(const control::MuteChangeResult &result)
{
    return buildMutePayload(result);
}

QJsonObject buildVolumePayload(const control::VolumeChangeResult &result)
{
    QJsonObject payload;
    payload[QStringLiteral("sinkId")] = result.sinkId;
    payload[QStringLiteral("requestedValue")] = numberOrNull(result.requestedValue);
    payload[QStringLiteral("targetValue")] = numberOrNull(result.targetValue);
    payload[QStringLiteral("previousValue")] = numberOrNull(result.previousValue);
    return payload;
}

QJsonObject buildVolumeErrorDetails(const control::VolumeChangeResult &result)
{
    return buildVolumePayload(result);
}

QJsonObject buildWindowActivationPayload(const control::WindowActivationResult &result)
{
    QJsonObject payload;
    payload[QStringLiteral("windowId")] = result.windowId;
    return payload;
}

QJsonObject buildWindowActivationErrorDetails(const control::WindowActivationResult &result)
{
    return buildWindowActivationPayload(result);
}

} // namespace plasma_bridge::api
