#include "adapters/audio/pulse_audio_volume_controller.h"

#include <PulseAudioQt/Context>
#include <PulseAudioQt/Sink>

#include <algorithm>
#include <cmath>

namespace plasma_bridge::audio
{
namespace
{

using ContextState = PulseAudioQt::Context::State;

constexpr double kMinimumNormalizedVolume = 0.0;
constexpr double kMaximumNormalizedVolume = 1.0;

control::VolumeChangeResult buildResult(control::VolumeChangeStatus status,
                                        const QString &sinkId,
                                        const std::optional<double> &requestedValue,
                                        const std::optional<double> &targetValue = std::nullopt,
                                        const std::optional<double> &previousValue = std::nullopt)
{
    control::VolumeChangeResult result;
    result.status = status;
    result.sinkId = sinkId;
    result.requestedValue = requestedValue;
    result.targetValue = targetValue;
    result.previousValue = previousValue;
    return result;
}

std::optional<double> finiteValueOrNull(const double value)
{
    return std::isfinite(value) ? std::make_optional(value) : std::nullopt;
}

double clampNormalizedVolume(const double value)
{
    return std::clamp(value, kMinimumNormalizedVolume, kMaximumNormalizedVolume);
}

double normalizedVolume(const PulseAudioQt::Sink *sink)
{
    return static_cast<double>(sink->volume()) / static_cast<double>(PulseAudioQt::normalVolume());
}

qint64 backendVolumeFromNormalized(const double value)
{
    const double clampedValue = clampNormalizedVolume(value);
    const auto roundedValue =
        static_cast<qint64>(std::llround(clampedValue * static_cast<double>(PulseAudioQt::normalVolume())));
    return std::clamp(roundedValue, PulseAudioQt::minimumVolume(), PulseAudioQt::normalVolume());
}

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

control::VolumeChangeResult setResolvedSinkVolume(PulseAudioQt::Sink *sink,
                                                  const QString &sinkId,
                                                  const std::optional<double> &requestedValue,
                                                  const double targetValue,
                                                  const double previousValue)
{
    if (!sink->isVolumeWritable()) {
        return buildResult(control::VolumeChangeStatus::SinkNotWritable,
                           sinkId,
                           requestedValue,
                           targetValue,
                           previousValue);
    }

    sink->setVolume(backendVolumeFromNormalized(targetValue));
    return buildResult(control::VolumeChangeStatus::Accepted,
                       sinkId,
                       requestedValue,
                       targetValue,
                       previousValue);
}

} // namespace

control::VolumeChangeResult PulseAudioVolumeController::setVolume(const QString &sinkId, const double value)
{
    const std::optional<double> requestedValue = finiteValueOrNull(value);
    if (!requestedValue.has_value()) {
        return buildResult(control::VolumeChangeStatus::InvalidValue, sinkId, requestedValue);
    }

    const double targetValue = clampNormalizedVolume(*requestedValue);
    PulseAudioQt::Context *context = readyContext();
    if (context == nullptr) {
        return buildResult(control::VolumeChangeStatus::NotReady, sinkId, requestedValue, targetValue);
    }

    PulseAudioQt::Sink *sink = findSink(context, sinkId);
    if (sink == nullptr) {
        return buildResult(control::VolumeChangeStatus::SinkNotFound, sinkId, requestedValue, targetValue);
    }

    return setResolvedSinkVolume(sink, sinkId, requestedValue, targetValue, normalizedVolume(sink));
}

control::VolumeChangeResult PulseAudioVolumeController::incrementVolume(const QString &sinkId, const double value)
{
    const std::optional<double> requestedValue = finiteValueOrNull(value);
    if (!requestedValue.has_value() || *requestedValue < 0.0) {
        return buildResult(control::VolumeChangeStatus::InvalidValue, sinkId, requestedValue);
    }

    PulseAudioQt::Context *context = readyContext();
    if (context == nullptr) {
        return buildResult(control::VolumeChangeStatus::NotReady, sinkId, requestedValue);
    }

    PulseAudioQt::Sink *sink = findSink(context, sinkId);
    if (sink == nullptr) {
        return buildResult(control::VolumeChangeStatus::SinkNotFound, sinkId, requestedValue);
    }

    const double previousValue = normalizedVolume(sink);
    const double targetValue = clampNormalizedVolume(previousValue + *requestedValue);
    return setResolvedSinkVolume(sink, sinkId, requestedValue, targetValue, previousValue);
}

control::VolumeChangeResult PulseAudioVolumeController::decrementVolume(const QString &sinkId, const double value)
{
    const std::optional<double> requestedValue = finiteValueOrNull(value);
    if (!requestedValue.has_value() || *requestedValue < 0.0) {
        return buildResult(control::VolumeChangeStatus::InvalidValue, sinkId, requestedValue);
    }

    PulseAudioQt::Context *context = readyContext();
    if (context == nullptr) {
        return buildResult(control::VolumeChangeStatus::NotReady, sinkId, requestedValue);
    }

    PulseAudioQt::Sink *sink = findSink(context, sinkId);
    if (sink == nullptr) {
        return buildResult(control::VolumeChangeStatus::SinkNotFound, sinkId, requestedValue);
    }

    const double previousValue = normalizedVolume(sink);
    const double targetValue = clampNormalizedVolume(previousValue - *requestedValue);
    return setResolvedSinkVolume(sink, sinkId, requestedValue, targetValue, previousValue);
}

} // namespace plasma_bridge::audio
