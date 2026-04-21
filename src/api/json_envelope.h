#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace plasma_bridge::control
{
struct DefaultDeviceChangeResult;
struct MuteChangeResult;
struct VolumeChangeResult;
}

namespace plasma_bridge::api
{

QJsonObject buildApiError(const QString &code, const QString &message, const QJsonObject &details = {});

QJsonDocument buildHttpSuccessEnvelope(const QJsonValue &payload);
QJsonDocument buildHttpErrorEnvelope(const QString &code, const QString &message, const QJsonObject &details = {});

QJsonObject buildWebSocketSuccessEnvelope(const QString &type, const QJsonValue &payload);
QJsonObject buildWebSocketErrorEnvelope(const QString &code, const QString &message, const QJsonObject &details = {});

QJsonObject buildDefaultDevicePayload(const control::DefaultDeviceChangeResult &result);
QJsonObject buildDefaultDeviceErrorDetails(const control::DefaultDeviceChangeResult &result);
QJsonObject buildMutePayload(const control::MuteChangeResult &result);
QJsonObject buildMuteErrorDetails(const control::MuteChangeResult &result);
QJsonObject buildVolumePayload(const control::VolumeChangeResult &result);
QJsonObject buildVolumeErrorDetails(const control::VolumeChangeResult &result);

} // namespace plasma_bridge::api
