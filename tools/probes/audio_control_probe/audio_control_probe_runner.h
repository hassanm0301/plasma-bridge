#pragma once

#include "control/audio_volume_controller.h"

#include <QObject>
#include <QString>

#include <optional>

class QCommandLineParser;
class QTextStream;
class QTimer;

namespace plasma_bridge::tools::audio_control_probe
{

inline constexpr int kDefaultStartupTimeoutMs = 5000;

enum class Command {
    Set,
    Increment,
    Decrement,
};

struct AudioControlProbeOptions {
    Command command = Command::Set;
    QString sinkId;
    double requestedValue = 0.0;
    int timeoutMs = kDefaultStartupTimeoutMs;
    bool jsonOutput = false;
};

struct ParseOptionsResult {
    bool ok = false;
    std::optional<AudioControlProbeOptions> options;
    QString errorMessage;
};

class AudioControlSubmissionGate : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~AudioControlSubmissionGate() override = default;

    virtual bool shouldSubmitRequest() const = 0;

signals:
    void submissionConditionChanged();
};

class PulseAudioSubmissionGate final : public AudioControlSubmissionGate
{
    Q_OBJECT

public:
    explicit PulseAudioSubmissionGate(QObject *parent = nullptr);

    bool shouldSubmitRequest() const override;

private:
    class Impl;
    Impl *m_impl = nullptr;
};

class AudioControlProbeRunner final : public QObject
{
    Q_OBJECT

public:
    explicit AudioControlProbeRunner(control::AudioVolumeController *controller,
                                     AudioControlSubmissionGate *submissionGate,
                                     const AudioControlProbeOptions &options,
                                     QTextStream *output,
                                     QTextStream *error,
                                     QObject *parent = nullptr);

    void start();

signals:
    void finished(int exitCode);

private:
    void finish(int exitCode);
    void submitRequest();

    control::AudioVolumeController *m_controller = nullptr;
    AudioControlSubmissionGate *m_submissionGate = nullptr;
    AudioControlProbeOptions m_options;
    QTextStream *m_output = nullptr;
    QTextStream *m_error = nullptr;
    bool m_finished = false;
    QTimer *m_startupTimer = nullptr;
};

void configureParser(QCommandLineParser &parser);
ParseOptionsResult parseOptions(const QCommandLineParser &parser);
std::optional<Command> parseCommand(const QString &value);
std::optional<double> parseRequestedValue(const QString &value);
QByteArray formatJsonResultBytes(const control::VolumeChangeResult &result);
QString formatHumanResultText(const control::VolumeChangeResult &result);
int exitCodeForResult(const control::VolumeChangeResult &result);

} // namespace plasma_bridge::tools::audio_control_probe
