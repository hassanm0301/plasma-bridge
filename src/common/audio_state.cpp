#include "common/audio_state.h"

#include <QJsonArray>
#include <QTextStream>

#include <algorithm>

namespace plasma_bridge
{
namespace
{

QJsonValue stringOrNull(const QString &value)
{
    return value.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(value);
}

QString boolText(bool value)
{
    return value ? QStringLiteral("yes") : QStringLiteral("no");
}

QString formatVolume(double normalizedVolume)
{
    return QStringLiteral("%1 (%2%)")
        .arg(QString::number(normalizedVolume, 'f', 4),
             QString::number(normalizedVolume * 100.0, 'f', 1));
}

void appendDevices(QTextStream &stream,
                   const QString &title,
                   const QList<plasma_bridge::AudioDeviceState> &devices)
{
    stream << title << " Count: " << devices.size() << '\n';

    for (const plasma_bridge::AudioDeviceState &device : devices) {
        stream << '\n';
        stream << (device.isDefault ? "* " : "  ") << device.label << '\n';
        stream << "  id: " << device.id << '\n';
        stream << "  volume: " << formatVolume(device.volume) << '\n';
        stream << "  muted: " << boolText(device.muted) << '\n';
        stream << "  available: " << boolText(device.available) << '\n';
        stream << "  virtual: " << boolText(device.isVirtual) << '\n';
        stream << "  backendApi: "
               << (device.backendApi.isEmpty() ? QStringLiteral("(none)") : device.backendApi) << '\n';
    }
}

} // namespace

AudioOutputState toAudioOutputState(const AudioState &state)
{
    AudioOutputState outputState;
    outputState.sinks = state.sinks;
    outputState.selectedSinkId = state.selectedSinkId;
    return outputState;
}

AudioInputState toAudioInputState(const AudioState &state)
{
    AudioInputState inputState;
    inputState.sources = state.sources;
    inputState.selectedSourceId = state.selectedSourceId;
    return inputState;
}

QJsonObject toJsonObject(const AudioDeviceState &device)
{
    QJsonObject json;
    json[QStringLiteral("id")] = device.id;
    json[QStringLiteral("label")] = device.label;
    json[QStringLiteral("volume")] = device.volume;
    json[QStringLiteral("muted")] = device.muted;
    json[QStringLiteral("available")] = device.available;
    json[QStringLiteral("isDefault")] = device.isDefault;
    json[QStringLiteral("isVirtual")] = device.isVirtual;
    json[QStringLiteral("backendApi")] = stringOrNull(device.backendApi);
    return json;
}

QJsonObject toJsonObject(const AudioOutputState &state)
{
    QJsonArray sinks;
    for (const AudioSinkState &sink : state.sinks) {
        sinks.append(toJsonObject(sink));
    }

    QJsonObject json;
    json[QStringLiteral("sinks")] = sinks;
    json[QStringLiteral("selectedSinkId")] = stringOrNull(state.selectedSinkId);
    return json;
}

QJsonObject toJsonObject(const AudioInputState &state)
{
    QJsonArray sources;
    for (const AudioSourceState &source : state.sources) {
        sources.append(toJsonObject(source));
    }

    QJsonObject json;
    json[QStringLiteral("sources")] = sources;
    json[QStringLiteral("selectedSourceId")] = stringOrNull(state.selectedSourceId);
    return json;
}

QJsonObject toJsonObject(const AudioState &state)
{
    QJsonObject json = toJsonObject(toAudioOutputState(state));
    json[QStringLiteral("sources")] = toJsonObject(toAudioInputState(state)).value(QStringLiteral("sources"));
    json[QStringLiteral("selectedSourceId")] = stringOrNull(state.selectedSourceId);
    return json;
}

QJsonObject toJsonEventObject(const QString &reason,
                              const QString &sinkId,
                              const QString &sourceId,
                              const AudioState &state)
{
    QJsonObject json;
    json[QStringLiteral("event")] = QStringLiteral("audioState");
    json[QStringLiteral("reason")] = reason;
    json[QStringLiteral("sinkId")] = stringOrNull(sinkId);
    json[QStringLiteral("sourceId")] = stringOrNull(sourceId);
    json[QStringLiteral("state")] = toJsonObject(state);
    return json;
}

QString formatHumanReadableEvent(const QString &reason,
                                 const QString &sinkId,
                                 const QString &sourceId,
                                 const AudioState &state)
{
    QString output;
    QTextStream stream(&output);

    stream << "Event: audioState\n";
    stream << "Reason: " << reason << '\n';
    stream << "Sink: " << (sinkId.isEmpty() ? QStringLiteral("(none)") : sinkId) << '\n';
    stream << "Source: " << (sourceId.isEmpty() ? QStringLiteral("(none)") : sourceId) << '\n';
    stream << "Selected Sink: "
           << (state.selectedSinkId.isEmpty() ? QStringLiteral("(none)") : state.selectedSinkId) << '\n';
    stream << "Selected Source: "
           << (state.selectedSourceId.isEmpty() ? QStringLiteral("(none)") : state.selectedSourceId) << '\n';

    appendDevices(stream, QStringLiteral("Sink"), state.sinks);
    stream << '\n';
    appendDevices(stream, QStringLiteral("Source"), state.sources);

    return output;
}

} // namespace plasma_bridge
