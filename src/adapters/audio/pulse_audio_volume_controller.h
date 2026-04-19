#pragma once

#include "control/audio_volume_controller.h"

namespace plasma_bridge::audio
{

class PulseAudioVolumeController final : public control::AudioVolumeController
{
public:
    control::VolumeChangeResult setVolume(const QString &sinkId, double value) override;
    control::VolumeChangeResult incrementVolume(const QString &sinkId, double value) override;
    control::VolumeChangeResult decrementVolume(const QString &sinkId, double value) override;
};

} // namespace plasma_bridge::audio
