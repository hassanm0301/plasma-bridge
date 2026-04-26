#pragma once

#include "plasma_bridge_build_config.h"
#include "common/audio_state.h"

#include <QObject>

class QCommandLineParser;
class QTextStream;
class QTimer;

namespace plasma_bridge::tools::audio_probe
{

struct AudioProbeOptions {
    bool watchMode = false;
    bool jsonOutput = false;
};

class AudioProbeSource : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~AudioProbeSource() override = default;

    virtual void start() = 0;
    virtual const plasma_bridge::AudioState &currentState() const = 0;
    virtual bool hasInitialState() const = 0;

signals:
    void initialStateReady();
    void audioStateChanged(const QString &reason, const QString &sinkId, const QString &sourceId);
    void connectionFailed(const QString &message);
};

class PulseAudioProbeSource final : public AudioProbeSource
{
    Q_OBJECT

public:
    explicit PulseAudioProbeSource(QObject *parent = nullptr);

    void start() override;
    const plasma_bridge::AudioState &currentState() const override;
    bool hasInitialState() const override;

private:
    class Impl;
    Impl *m_impl = nullptr;
};

class AudioProbeRunner final : public QObject
{
    Q_OBJECT

public:
    explicit AudioProbeRunner(AudioProbeSource *source,
                              const AudioProbeOptions &options,
                              QTextStream *output,
                              QTextStream *error,
                              int startupTimeoutMs = PLASMA_BRIDGE_DEFAULT_PROBE_STARTUP_TIMEOUT_MS,
                              QObject *parent = nullptr);

    void start();

signals:
    void finished(int exitCode);

private:
    void finish(int exitCode);
    void publishInitialState();
    void printEvent(const QString &reason, const QString &sinkId, const QString &sourceId, bool compactOutput) const;

    AudioProbeSource *m_source = nullptr;
    AudioProbeOptions m_options;
    QTextStream *m_output = nullptr;
    QTextStream *m_error = nullptr;
    int m_startupTimeoutMs = 0;
    bool m_finished = false;
    bool m_initialStatePublished = false;
    QTimer *m_startupTimer = nullptr;
};

void configureParser(QCommandLineParser &parser);
AudioProbeOptions optionsFromParser(const QCommandLineParser &parser);
QByteArray formatJsonEventBytes(const QString &reason,
                                const QString &sinkId,
                                const QString &sourceId,
                                const plasma_bridge::AudioState &state,
                                bool compactOutput);
QString formatHumanEventText(const QString &reason,
                             const QString &sinkId,
                             const QString &sourceId,
                             const plasma_bridge::AudioState &state);

} // namespace plasma_bridge::tools::audio_probe
