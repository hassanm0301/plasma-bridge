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
}

namespace plasma_bridge::audio
{

class PulseAudioSinkObserver final : public QObject
{
    Q_OBJECT

public:
    explicit PulseAudioSinkObserver(QObject *parent = nullptr);

    void start();

    const plasma_bridge::AudioState &currentState() const;
    bool hasInitialState() const;
    bool isPipeWireServer() const;
    QString serverVersion() const;

signals:
    void initialStateReady();
    void audioStateChanged(const QString &reason, const QString &sinkId);
    void connectionFailed(const QString &message);

private:
    void attachSink(PulseAudioQt::Sink *sink);
    void scheduleInitialPublish();
    void refreshAndEmit(const QString &reason, const QString &sinkId = {});
    plasma_bridge::AudioState buildState() const;
    void handleContextStateChanged();

    PulseAudioQt::Context *m_context = nullptr;
    PulseAudioQt::Server *m_server = nullptr;
    plasma_bridge::AudioState m_state;
    QSet<const QObject *> m_attachedSinks;
    QTimer m_initialPublishTimer;
    bool m_started = false;
    bool m_initialStatePublished = false;
};

} // namespace plasma_bridge::audio
