#pragma once

#include "common/audio_state.h"

#include <QObject>
#include <QSet>
#include <QTimer>

namespace PulseAudioQt
{
class Context;
class Server;
class Sink;
class Source;
}

namespace plasma_bridge::audio
{

class PulseAudioStateObserver final : public QObject
{
    Q_OBJECT

public:
    explicit PulseAudioStateObserver(QObject *parent = nullptr);

    void start();

    const plasma_bridge::AudioState &currentState() const;
    bool hasInitialState() const;
    bool isPipeWireServer() const;
    QString serverVersion() const;

signals:
    void initialStateReady();
    void audioStateChanged(const QString &reason, const QString &sinkId, const QString &sourceId);
    void connectionFailed(const QString &message);

private:
    void attachSink(PulseAudioQt::Sink *sink);
    void attachSource(PulseAudioQt::Source *source);
    void scheduleInitialPublish();
    void refreshAndEmit(const QString &reason, const QString &sinkId = {}, const QString &sourceId = {});
    plasma_bridge::AudioState buildState() const;
    void handleContextStateChanged();

    PulseAudioQt::Context *m_context = nullptr;
    PulseAudioQt::Server *m_server = nullptr;
    plasma_bridge::AudioState m_state;
    QSet<const QObject *> m_attachedDevices;
    QTimer m_initialPublishTimer;
    bool m_started = false;
    bool m_initialStatePublished = false;
};

} // namespace plasma_bridge::audio
