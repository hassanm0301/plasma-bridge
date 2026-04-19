#pragma once

#include "control/audio_device_controller.h"
#include "control/audio_volume_controller.h"

#include <QByteArray>
#include <QString>

namespace plasma_bridge::api
{

enum class AudioControlTargetKind {
    Sink,
    Source,
};

enum class AudioControlOperation {
    SetVolume,
    IncrementVolume,
    DecrementVolume,
    SetDefault,
    SetMute,
};

enum class AudioControlRouteMatch {
    NoMatch,
    Match,
    InvalidSinkId,
    InvalidSourceId,
};

struct AudioControlRoute {
    AudioControlTargetKind targetKind = AudioControlTargetKind::Sink;
    AudioControlOperation operation = AudioControlOperation::SetVolume;
    QString deviceId;
};

struct AudioControlRouteParseResult {
    AudioControlRouteMatch match = AudioControlRouteMatch::NoMatch;
    AudioControlRoute route;
};

AudioControlRouteParseResult parseAudioControlRoute(const QString &path);
int httpStatusCodeForVolumeChangeStatus(control::VolumeChangeStatus status);
int httpStatusCodeForAudioDeviceChangeStatus(control::AudioDeviceChangeStatus status);
QByteArray reasonPhraseForStatusCode(int statusCode);

} // namespace plasma_bridge::api
