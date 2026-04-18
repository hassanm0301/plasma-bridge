#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>

namespace plasma_bridge
{

struct AudioSinkState {
    QString id;
    QString label;
    double volume = 0.0;
    bool muted = false;
    bool available = false;
    bool isDefault = false;
    bool isVirtual = false;
    QString backendApi;
};

struct AudioState {
    QList<AudioSinkState> sinks;
    QString selectedSinkId;
};

QJsonObject toJsonObject(const AudioSinkState &sink);
QJsonObject toJsonObject(const AudioState &state);
QJsonObject toJsonEventObject(const QString &reason, const QString &sinkId, const AudioState &state);

QString formatHumanReadableEvent(const QString &reason, const QString &sinkId, const AudioState &state);

} // namespace plasma_bridge
