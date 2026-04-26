#pragma once

#include <QJsonObject>
#include <QString>

#include <optional>

namespace plasma_bridge::control
{

enum class VolumeChangeStatus {
    Accepted,
    NotReady,
    SinkNotFound,
    SinkNotWritable,
    InvalidValue,
};

struct VolumeChangeResult {
    VolumeChangeStatus status = VolumeChangeStatus::InvalidValue;
    QString sinkId;
    std::optional<double> requestedValue;
    std::optional<double> targetValue;
    std::optional<double> previousValue;
};

class AudioVolumeController
{
public:
    virtual ~AudioVolumeController() = default;

    virtual VolumeChangeResult setVolume(const QString &sinkId, double value) = 0;
    virtual VolumeChangeResult incrementVolume(const QString &sinkId, double value) = 0;
    virtual VolumeChangeResult decrementVolume(const QString &sinkId, double value) = 0;
};

QString volumeChangeStatusName(VolumeChangeStatus status);
QJsonObject toJsonObject(const VolumeChangeResult &result);
QString formatHumanReadableResult(const VolumeChangeResult &result);

} // namespace plasma_bridge::control
