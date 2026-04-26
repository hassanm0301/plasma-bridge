#pragma once

#include <QJsonObject>
#include <QString>

#include <optional>

namespace plasma_bridge::control
{

enum class AudioDeviceChangeStatus {
    Accepted,
    NotReady,
    SinkNotFound,
    SourceNotFound,
    SinkNotWritable,
    SourceNotWritable,
};

struct DefaultDeviceChangeResult {
    AudioDeviceChangeStatus status = AudioDeviceChangeStatus::NotReady;
    QString deviceId;
};

struct MuteChangeResult {
    AudioDeviceChangeStatus status = AudioDeviceChangeStatus::NotReady;
    QString deviceId;
    std::optional<bool> requestedMuted;
    std::optional<bool> targetMuted;
    std::optional<bool> previousMuted;
};

class AudioDeviceController
{
public:
    virtual ~AudioDeviceController() = default;

    virtual DefaultDeviceChangeResult setDefaultSink(const QString &sinkId) = 0;
    virtual DefaultDeviceChangeResult setDefaultSource(const QString &sourceId) = 0;
    virtual MuteChangeResult setSinkMuted(const QString &sinkId, bool muted) = 0;
    virtual MuteChangeResult setSourceMuted(const QString &sourceId, bool muted) = 0;
};

QString audioDeviceChangeStatusName(AudioDeviceChangeStatus status);
QJsonObject toJsonObject(const DefaultDeviceChangeResult &result);
QJsonObject toJsonObject(const MuteChangeResult &result);
QString formatHumanReadableResult(const DefaultDeviceChangeResult &result);
QString formatHumanReadableResult(const MuteChangeResult &result);

} // namespace plasma_bridge::control
