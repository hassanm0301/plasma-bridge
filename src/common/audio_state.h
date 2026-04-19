#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>

namespace plasma_bridge
{

struct AudioDeviceState {
    QString id;
    QString label;
    double volume = 0.0;
    bool muted = false;
    bool available = false;
    bool isDefault = false;
    bool isVirtual = false;
    QString backendApi;
};

using AudioSinkState = AudioDeviceState;
using AudioSourceState = AudioDeviceState;

struct AudioOutputState {
    QList<AudioSinkState> sinks;
    QString selectedSinkId;
};

struct AudioInputState {
    QList<AudioSourceState> sources;
    QString selectedSourceId;
};

struct AudioState {
    QList<AudioSinkState> sinks;
    QString selectedSinkId;
    QList<AudioSourceState> sources;
    QString selectedSourceId;
};

AudioOutputState toAudioOutputState(const AudioState &state);
AudioInputState toAudioInputState(const AudioState &state);

QJsonObject toJsonObject(const AudioDeviceState &device);
QJsonObject toJsonObject(const AudioOutputState &state);
QJsonObject toJsonObject(const AudioInputState &state);
QJsonObject toJsonObject(const AudioState &state);
QJsonObject toJsonEventObject(const QString &reason,
                              const QString &sinkId,
                              const QString &sourceId,
                              const AudioState &state);

QString formatHumanReadableEvent(const QString &reason,
                                 const QString &sinkId,
                                 const QString &sourceId,
                                 const AudioState &state);

} // namespace plasma_bridge
