#pragma once

#include "control/audio_device_controller.h"

namespace plasma_bridge::audio
{

class PulseAudioDeviceController final : public control::AudioDeviceController
{
public:
    control::DefaultDeviceChangeResult setDefaultSink(const QString &sinkId) override;
    control::DefaultDeviceChangeResult setDefaultSource(const QString &sourceId) override;
    control::MuteChangeResult setSinkMuted(const QString &sinkId, bool muted) override;
    control::MuteChangeResult setSourceMuted(const QString &sourceId, bool muted) override;
};

} // namespace plasma_bridge::audio
