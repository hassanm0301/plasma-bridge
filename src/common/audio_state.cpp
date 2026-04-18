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

} // namespace

QJsonObject toJsonObject(const AudioSinkState &sink)
{
    QJsonObject json;
    json[QStringLiteral("id")] = sink.id;
    json[QStringLiteral("label")] = sink.label;
    json[QStringLiteral("volume")] = sink.volume;
    json[QStringLiteral("muted")] = sink.muted;
    json[QStringLiteral("available")] = sink.available;
    json[QStringLiteral("isDefault")] = sink.isDefault;
    json[QStringLiteral("isVirtual")] = sink.isVirtual;
    json[QStringLiteral("backendApi")] = stringOrNull(sink.backendApi);
    return json;
}

QJsonObject toJsonObject(const AudioState &state)
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

QJsonObject toJsonEventObject(const QString &reason, const QString &sinkId, const AudioState &state)
{
    QJsonObject json;
    json[QStringLiteral("event")] = QStringLiteral("audioState");
    json[QStringLiteral("reason")] = reason;
    json[QStringLiteral("sinkId")] = stringOrNull(sinkId);
    json[QStringLiteral("state")] = toJsonObject(state);
    return json;
}

QString formatHumanReadableEvent(const QString &reason, const QString &sinkId, const AudioState &state)
{
    QString output;
    QTextStream stream(&output);

    stream << "Event: audioState\n";
    stream << "Reason: " << reason << '\n';
    stream << "Sink: " << (sinkId.isEmpty() ? QStringLiteral("(none)") : sinkId) << '\n';
    stream << "Selected Sink: "
           << (state.selectedSinkId.isEmpty() ? QStringLiteral("(none)") : state.selectedSinkId) << '\n';
    stream << "Sink Count: " << state.sinks.size() << '\n';

    for (const AudioSinkState &sink : state.sinks) {
        stream << '\n';
        stream << (sink.isDefault ? "* " : "  ") << sink.label << '\n';
        stream << "  id: " << sink.id << '\n';
        stream << "  volume: " << formatVolume(sink.volume) << '\n';
        stream << "  muted: " << boolText(sink.muted) << '\n';
        stream << "  available: " << boolText(sink.available) << '\n';
        stream << "  virtual: " << boolText(sink.isVirtual) << '\n';
        stream << "  backendApi: "
               << (sink.backendApi.isEmpty() ? QStringLiteral("(none)") : sink.backendApi) << '\n';
    }

    return output;
}

} // namespace plasma_bridge
