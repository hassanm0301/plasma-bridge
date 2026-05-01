#pragma once

#include "common/media_state.h"
#include "control/media_controller.h"
#include "plasma_bridge_build_config.h"

#include <QObject>

#include <optional>
#include <memory>

class QCommandLineParser;
class QTextStream;
class QTimer;

namespace plasma_bridge::media
{
class MprisMediaObserver;
}

namespace plasma_bridge::tools::media_probe
{

enum class Command {
    Current,
    Play,
    Pause,
    PlayPause,
    Next,
    Previous,
};

struct MediaProbeOptions {
    Command command = Command::Current;
    bool watchMode = false;
    bool jsonOutput = false;
    int timeoutMs = PLASMA_BRIDGE_DEFAULT_PROBE_STARTUP_TIMEOUT_MS;
};

struct ParseOptionsResult {
    bool ok = false;
    std::optional<MediaProbeOptions> options;
    QString errorMessage;
};

class MediaProbeSource : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~MediaProbeSource() override = default;

    virtual void start() = 0;
    virtual const plasma_bridge::MediaState &currentState() const = 0;
    virtual bool hasInitialState() const = 0;

signals:
    void initialStateReady();
    void mediaStateChanged(const QString &reason, const QString &playerId);
    void connectionFailed(const QString &message);
};

class MprisMediaProbeSource final : public MediaProbeSource
{
    Q_OBJECT

public:
    explicit MprisMediaProbeSource(media::MprisMediaObserver *observer = nullptr, QObject *parent = nullptr);
    ~MprisMediaProbeSource() override;

    void start() override;
    const plasma_bridge::MediaState &currentState() const override;
    bool hasInitialState() const override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

class MediaProbeRunner final : public QObject
{
    Q_OBJECT

public:
    explicit MediaProbeRunner(MediaProbeSource *source,
                              control::MediaController *controller,
                              const MediaProbeOptions &options,
                              QTextStream *output,
                              QTextStream *error,
                              QObject *parent = nullptr);

    void start();

signals:
    void finished(int exitCode);

private:
    void executeControlCommand();
    void finish(int exitCode);
    void publishInitialState();
    void printCurrentState(const QString &reason, const QString &playerId, bool compactOutput) const;
    void printControlResult(const control::MediaControlResult &result) const;

    MediaProbeSource *m_source = nullptr;
    control::MediaController *m_controller = nullptr;
    MediaProbeOptions m_options;
    QTextStream *m_output = nullptr;
    QTextStream *m_error = nullptr;
    bool m_finished = false;
    bool m_initialStatePublished = false;
    QTimer *m_startupTimer = nullptr;
};

void configureParser(QCommandLineParser &parser);
ParseOptionsResult parseOptions(const QCommandLineParser &parser);
std::optional<Command> parseCommand(const QString &value);
QByteArray formatJsonEventBytes(const QString &reason,
                                const QString &playerId,
                                const plasma_bridge::MediaState &state,
                                bool compactOutput);
QString formatHumanEventText(const QString &reason, const QString &playerId, const plasma_bridge::MediaState &state);
QByteArray formatJsonControlBytes(Command command,
                                  const control::MediaControlResult &result,
                                  bool compactOutput);
QString formatHumanControlText(Command command, const control::MediaControlResult &result);
QString commandName(Command command);

} // namespace plasma_bridge::tools::media_probe
