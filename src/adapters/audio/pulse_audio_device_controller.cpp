#include "adapters/audio/pulse_audio_device_controller.h"

#include <PulseAudioQt/Context>
#include <PulseAudioQt/Server>
#include <PulseAudioQt/Sink>
#include <PulseAudioQt/Source>

namespace plasma_bridge::audio
{
namespace
{

using ContextState = PulseAudioQt::Context::State;

PulseAudioQt::Context *readyContext()
{
    PulseAudioQt::Context *context = PulseAudioQt::Context::instance();
    if (context == nullptr || context->state() != ContextState::Ready) {
        return nullptr;
    }

    return context;
}

PulseAudioQt::Sink *findSink(PulseAudioQt::Context *context, const QString &sinkId)
{
    if (context == nullptr) {
        return nullptr;
    }

    for (PulseAudioQt::Sink *sink : context->sinks()) {
        if (sink != nullptr && sink->name() == sinkId) {
            return sink;
        }
    }

    return nullptr;
}

PulseAudioQt::Source *findSource(PulseAudioQt::Context *context, const QString &sourceId)
{
    if (context == nullptr) {
        return nullptr;
    }

    for (PulseAudioQt::Source *source : context->sources()) {
        if (source != nullptr && source->name() == sourceId) {
            return source;
        }
    }

    return nullptr;
}

control::DefaultDeviceChangeResult buildDefaultResult(const control::AudioDeviceChangeStatus status, const QString &deviceId)
{
    control::DefaultDeviceChangeResult result;
    result.status = status;
    result.deviceId = deviceId;
    return result;
}

control::MuteChangeResult buildMuteResult(const control::AudioDeviceChangeStatus status,
                                          const QString &deviceId,
                                          const std::optional<bool> &requestedMuted,
                                          const std::optional<bool> &targetMuted = std::nullopt,
                                          const std::optional<bool> &previousMuted = std::nullopt)
{
    control::MuteChangeResult result;
    result.status = status;
    result.deviceId = deviceId;
    result.requestedMuted = requestedMuted;
    result.targetMuted = targetMuted;
    result.previousMuted = previousMuted;
    return result;
}

template<typename Device>
control::MuteChangeResult setResolvedDeviceMuted(Device *device,
                                                 const QString &deviceId,
                                                 const bool muted,
                                                 const control::AudioDeviceChangeStatus notWritableStatus)
{
    if (!device->isVolumeWritable()) {
        return buildMuteResult(notWritableStatus, deviceId, muted, device->isMuted(), device->isMuted());
    }

    const bool previousMuted = device->isMuted();
    device->setMuted(muted);
    return buildMuteResult(control::AudioDeviceChangeStatus::Accepted, deviceId, muted, muted, previousMuted);
}

} // namespace

control::DefaultDeviceChangeResult PulseAudioDeviceController::setDefaultSink(const QString &sinkId)
{
    PulseAudioQt::Context *context = readyContext();
    if (context == nullptr) {
        return buildDefaultResult(control::AudioDeviceChangeStatus::NotReady, sinkId);
    }

    PulseAudioQt::Server *server = context->server();
    if (server == nullptr) {
        return buildDefaultResult(control::AudioDeviceChangeStatus::NotReady, sinkId);
    }

    PulseAudioQt::Sink *sink = findSink(context, sinkId);
    if (sink == nullptr) {
        return buildDefaultResult(control::AudioDeviceChangeStatus::SinkNotFound, sinkId);
    }

    server->setDefaultSink(sink);
    return buildDefaultResult(control::AudioDeviceChangeStatus::Accepted, sinkId);
}

control::DefaultDeviceChangeResult PulseAudioDeviceController::setDefaultSource(const QString &sourceId)
{
    PulseAudioQt::Context *context = readyContext();
    if (context == nullptr) {
        return buildDefaultResult(control::AudioDeviceChangeStatus::NotReady, sourceId);
    }

    PulseAudioQt::Server *server = context->server();
    if (server == nullptr) {
        return buildDefaultResult(control::AudioDeviceChangeStatus::NotReady, sourceId);
    }

    PulseAudioQt::Source *source = findSource(context, sourceId);
    if (source == nullptr) {
        return buildDefaultResult(control::AudioDeviceChangeStatus::SourceNotFound, sourceId);
    }

    server->setDefaultSource(source);
    return buildDefaultResult(control::AudioDeviceChangeStatus::Accepted, sourceId);
}

control::MuteChangeResult PulseAudioDeviceController::setSinkMuted(const QString &sinkId, const bool muted)
{
    PulseAudioQt::Context *context = readyContext();
    if (context == nullptr) {
        return buildMuteResult(control::AudioDeviceChangeStatus::NotReady, sinkId, muted);
    }

    PulseAudioQt::Sink *sink = findSink(context, sinkId);
    if (sink == nullptr) {
        return buildMuteResult(control::AudioDeviceChangeStatus::SinkNotFound, sinkId, muted);
    }

    return setResolvedDeviceMuted(sink, sinkId, muted, control::AudioDeviceChangeStatus::SinkNotWritable);
}

control::MuteChangeResult PulseAudioDeviceController::setSourceMuted(const QString &sourceId, const bool muted)
{
    PulseAudioQt::Context *context = readyContext();
    if (context == nullptr) {
        return buildMuteResult(control::AudioDeviceChangeStatus::NotReady, sourceId, muted);
    }

    PulseAudioQt::Source *source = findSource(context, sourceId);
    if (source == nullptr) {
        return buildMuteResult(control::AudioDeviceChangeStatus::SourceNotFound, sourceId, muted);
    }

    return setResolvedDeviceMuted(source, sourceId, muted, control::AudioDeviceChangeStatus::SourceNotWritable);
}

} // namespace plasma_bridge::audio
