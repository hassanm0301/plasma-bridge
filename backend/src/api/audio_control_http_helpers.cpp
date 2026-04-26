#include "api/audio_control_http_helpers.h"

#include <QUrl>

namespace plasma_bridge::api
{
namespace
{

const QString kControlSinksPrefix = QStringLiteral("/control/audio/sinks/");
const QString kControlSourcesPrefix = QStringLiteral("/control/audio/sources/");
const QString kVolumePathSuffix = QStringLiteral("/volume");
const QString kVolumeIncrementPathSuffix = QStringLiteral("/volume/increment");
const QString kVolumeDecrementPathSuffix = QStringLiteral("/volume/decrement");
const QString kDefaultPathSuffix = QStringLiteral("/default");
const QString kMutePathSuffix = QStringLiteral("/mute");

AudioControlRouteParseResult parseRouteForPrefix(const QString &path,
                                                 const QString &prefix,
                                                 const AudioControlTargetKind targetKind)
{
    AudioControlRouteParseResult result;
    if (!path.startsWith(prefix)) {
        return result;
    }

    const auto trySuffix = [&](const QString &suffix, const AudioControlOperation operation) -> bool {
        if (!path.endsWith(suffix)) {
            return false;
        }

        const qsizetype encodedDeviceIdLength = path.size() - prefix.size() - suffix.size();
        if (encodedDeviceIdLength <= 0) {
            return false;
        }

        const QString encodedDeviceId = path.mid(prefix.size(), encodedDeviceIdLength);
        if (encodedDeviceId.contains(QLatin1Char('/'))) {
            result.match = targetKind == AudioControlTargetKind::Sink ? AudioControlRouteMatch::InvalidSinkId
                                                                      : AudioControlRouteMatch::InvalidSourceId;
            return true;
        }

        const QString deviceId = QUrl::fromPercentEncoding(encodedDeviceId.toUtf8());
        if (deviceId.isEmpty() || deviceId.contains(QLatin1Char('/'))) {
            result.match = targetKind == AudioControlTargetKind::Sink ? AudioControlRouteMatch::InvalidSinkId
                                                                      : AudioControlRouteMatch::InvalidSourceId;
            return true;
        }

        result.match = AudioControlRouteMatch::Match;
        result.route.targetKind = targetKind;
        result.route.operation = operation;
        result.route.deviceId = deviceId;
        return true;
    };

    if (targetKind == AudioControlTargetKind::Sink) {
        if (trySuffix(kVolumeIncrementPathSuffix, AudioControlOperation::IncrementVolume)) {
            return result;
        }
        if (trySuffix(kVolumeDecrementPathSuffix, AudioControlOperation::DecrementVolume)) {
            return result;
        }
        if (trySuffix(kVolumePathSuffix, AudioControlOperation::SetVolume)) {
            return result;
        }
    }

    if (trySuffix(kDefaultPathSuffix, AudioControlOperation::SetDefault)) {
        return result;
    }
    if (trySuffix(kMutePathSuffix, AudioControlOperation::SetMute)) {
        return result;
    }

    return result;
}

} // namespace

AudioControlRouteParseResult parseAudioControlRoute(const QString &path)
{
    AudioControlRouteParseResult result = parseRouteForPrefix(path, kControlSinksPrefix, AudioControlTargetKind::Sink);
    if (result.match != AudioControlRouteMatch::NoMatch) {
        return result;
    }

    return parseRouteForPrefix(path, kControlSourcesPrefix, AudioControlTargetKind::Source);
}

int httpStatusCodeForVolumeChangeStatus(const control::VolumeChangeStatus status)
{
    switch (status) {
    case control::VolumeChangeStatus::Accepted:
        return 200;
    case control::VolumeChangeStatus::InvalidValue:
        return 400;
    case control::VolumeChangeStatus::SinkNotFound:
        return 404;
    case control::VolumeChangeStatus::SinkNotWritable:
        return 409;
    case control::VolumeChangeStatus::NotReady:
        return 503;
    }

    return 400;
}

int httpStatusCodeForAudioDeviceChangeStatus(const control::AudioDeviceChangeStatus status)
{
    switch (status) {
    case control::AudioDeviceChangeStatus::Accepted:
        return 200;
    case control::AudioDeviceChangeStatus::SinkNotFound:
    case control::AudioDeviceChangeStatus::SourceNotFound:
        return 404;
    case control::AudioDeviceChangeStatus::SinkNotWritable:
    case control::AudioDeviceChangeStatus::SourceNotWritable:
        return 409;
    case control::AudioDeviceChangeStatus::NotReady:
        return 503;
    }

    return 400;
}

QByteArray reasonPhraseForStatusCode(const int statusCode)
{
    switch (statusCode) {
    case 200:
        return QByteArrayLiteral("OK");
    case 400:
        return QByteArrayLiteral("Bad Request");
    case 404:
        return QByteArrayLiteral("Not Found");
    case 405:
        return QByteArrayLiteral("Method Not Allowed");
    case 409:
        return QByteArrayLiteral("Conflict");
    case 415:
        return QByteArrayLiteral("Unsupported Media Type");
    case 503:
        return QByteArrayLiteral("Service Unavailable");
    default:
        return QByteArrayLiteral("Bad Request");
    }
}

} // namespace plasma_bridge::api
