#include "tools/probes/media_probe/media_probe_runner.h"

#include "adapters/media/mpris_media_controller.h"
#include "adapters/media/mpris_media_observer.h"
#include "state/media_state_store.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QTimer>

namespace plasma_bridge::tools::media_probe
{
namespace
{

bool commandSupportsWatch(const Command command)
{
    return command == Command::Current;
}

int exitCodeForResult(const control::MediaControlResult &result)
{
    return result.status == control::MediaControlStatus::Accepted ? 0 : 1;
}

} // namespace

class MprisMediaProbeSource::Impl final : public QObject
{
    Q_OBJECT

public:
    explicit Impl(MprisMediaProbeSource *owner, media::MprisMediaObserver *observer)
        : QObject(owner)
        , m_observer(observer == nullptr ? new media::MprisMediaObserver(this) : observer)
        , m_ownsObserver(observer == nullptr)
    {
        connect(m_observer, &media::MprisMediaObserver::initialStateReady, owner, &MediaProbeSource::initialStateReady);
        connect(m_observer, &media::MprisMediaObserver::mediaStateChanged, owner, &MediaProbeSource::mediaStateChanged);
        connect(m_observer, &media::MprisMediaObserver::connectionFailed, owner, &MediaProbeSource::connectionFailed);
    }

    media::MprisMediaObserver *m_observer = nullptr;
    bool m_ownsObserver = false;
};

MprisMediaProbeSource::MprisMediaProbeSource(media::MprisMediaObserver *observer, QObject *parent)
    : MediaProbeSource(parent)
    , m_impl(std::make_unique<Impl>(this, observer))
{
}

MprisMediaProbeSource::~MprisMediaProbeSource() = default;

void MprisMediaProbeSource::start()
{
    m_impl->m_observer->start();
}

const plasma_bridge::MediaState &MprisMediaProbeSource::currentState() const
{
    return m_impl->m_observer->currentState();
}

bool MprisMediaProbeSource::hasInitialState() const
{
    return m_impl->m_observer->hasInitialState();
}

MediaProbeRunner::MediaProbeRunner(MediaProbeSource *source,
                                   control::MediaController *controller,
                                   const MediaProbeOptions &options,
                                   QTextStream *output,
                                   QTextStream *error,
                                   QObject *parent)
    : QObject(parent)
    , m_source(source)
    , m_controller(controller)
    , m_options(options)
    , m_output(output)
    , m_error(error)
    , m_startupTimer(new QTimer(this))
{
    m_startupTimer->setSingleShot(true);

    connect(m_startupTimer, &QTimer::timeout, this, [this]() {
        if (m_source != nullptr && !m_source->hasInitialState()) {
            if (m_error != nullptr) {
                *m_error << "Timed out waiting for MPRIS media state." << Qt::endl;
            }
            finish(1);
        }
    });

    if (m_source == nullptr) {
        return;
    }

    connect(m_source, &MediaProbeSource::connectionFailed, this, [this](const QString &message) {
        if (m_error != nullptr) {
            *m_error << message << Qt::endl;
        }
        if (!m_source->hasInitialState()) {
            finish(1);
        }
    });

    connect(m_source, &MediaProbeSource::initialStateReady, this, [this]() {
        publishInitialState();
    });

    connect(m_source, &MediaProbeSource::mediaStateChanged, this, [this](const QString &reason, const QString &playerId) {
        if (!m_options.watchMode || m_options.command != Command::Current || m_source == nullptr) {
            return;
        }

        printCurrentState(reason, playerId, true);
    });
}

void MediaProbeRunner::start()
{
    if (m_finished || m_source == nullptr) {
        if (!m_finished) {
            finish(1);
        }
        return;
    }

    if (m_options.timeoutMs >= 0) {
        m_startupTimer->start(m_options.timeoutMs);
    }

    m_source->start();
    if (m_source->hasInitialState()) {
        publishInitialState();
    }
}

void MediaProbeRunner::executeControlCommand()
{
    if (m_controller == nullptr) {
        if (m_error != nullptr) {
            *m_error << "media_probe controller is unavailable." << Qt::endl;
        }
        finish(1);
        return;
    }

    control::MediaControlResult result;
    switch (m_options.command) {
    case Command::Current:
        finish(0);
        return;
    case Command::Play:
        result = m_controller->play();
        break;
    case Command::Pause:
        result = m_controller->pause();
        break;
    case Command::PlayPause:
        result = m_controller->playPause();
        break;
    case Command::Next:
        result = m_controller->next();
        break;
    case Command::Previous:
        result = m_controller->previous();
        break;
    }

    printControlResult(result);
    finish(exitCodeForResult(result));
}

void MediaProbeRunner::finish(const int exitCode)
{
    if (m_finished) {
        return;
    }

    m_finished = true;
    m_startupTimer->stop();
    emit finished(exitCode);
}

void MediaProbeRunner::publishInitialState()
{
    if (m_initialStatePublished || m_source == nullptr || !m_source->hasInitialState()) {
        return;
    }

    m_initialStatePublished = true;
    m_startupTimer->stop();

    if (m_options.command == Command::Current) {
        printCurrentState(QStringLiteral("initial"), QString(), m_options.watchMode);
        if (!m_options.watchMode) {
            finish(0);
        }
        return;
    }

    executeControlCommand();
}

void MediaProbeRunner::printCurrentState(const QString &reason, const QString &playerId, const bool compactOutput) const
{
    if (m_output == nullptr || m_source == nullptr) {
        return;
    }

    if (m_options.jsonOutput) {
        *m_output << formatJsonEventBytes(reason, playerId, m_source->currentState(), compactOutput) << Qt::endl;
        return;
    }

    *m_output << formatHumanEventText(reason, playerId, m_source->currentState()) << Qt::endl;
}

void MediaProbeRunner::printControlResult(const control::MediaControlResult &result) const
{
    if (m_output == nullptr) {
        return;
    }

    if (m_options.jsonOutput) {
        *m_output << formatJsonControlBytes(m_options.command, result, false) << Qt::endl;
        return;
    }

    *m_output << formatHumanControlText(m_options.command, result) << Qt::endl;
}

void configureParser(QCommandLineParser &parser)
{
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption(QStringLiteral("watch"),
                                        QStringLiteral("Keep running and print live media updates.")));
    parser.addOption(QCommandLineOption(QStringLiteral("json"),
                                        QStringLiteral("Print machine-readable JSON output.")));
    parser.addOption(QCommandLineOption(QStringLiteral("timeout-ms"),
                                        QStringLiteral("Startup timeout in milliseconds."),
                                        QStringLiteral("timeout-ms"),
                                        QString::number(PLASMA_BRIDGE_DEFAULT_PROBE_STARTUP_TIMEOUT_MS)));
    parser.addPositionalArgument(QStringLiteral("command"),
                                 QStringLiteral("Optional command: current, play, pause, play-pause, next, previous."));
}

ParseOptionsResult parseOptions(const QCommandLineParser &parser)
{
    ParseOptionsResult result;
    MediaProbeOptions options;
    options.watchMode = parser.isSet(QStringLiteral("watch"));
    options.jsonOutput = parser.isSet(QStringLiteral("json"));

    bool ok = false;
    const int timeoutMs = parser.value(QStringLiteral("timeout-ms")).toInt(&ok);
    if (!ok || timeoutMs < 0) {
        result.errorMessage = QStringLiteral("timeout-ms must be a non-negative integer.");
        return result;
    }
    options.timeoutMs = timeoutMs;

    const QStringList positional = parser.positionalArguments();
    if (positional.size() > 1) {
        result.errorMessage = QStringLiteral("media_probe accepts at most one command.");
        return result;
    }

    if (!positional.isEmpty()) {
        const std::optional<Command> command = parseCommand(positional.first());
        if (!command.has_value()) {
            result.errorMessage = QStringLiteral("Unsupported command: %1").arg(positional.first());
            return result;
        }
        options.command = *command;
    }

    if (options.watchMode && !commandSupportsWatch(options.command)) {
        result.errorMessage = QStringLiteral("--watch is only supported with the current command.");
        return result;
    }

    result.ok = true;
    result.options = options;
    return result;
}

std::optional<Command> parseCommand(const QString &value)
{
    if (value == QStringLiteral("current")) {
        return Command::Current;
    }
    if (value == QStringLiteral("play")) {
        return Command::Play;
    }
    if (value == QStringLiteral("pause")) {
        return Command::Pause;
    }
    if (value == QStringLiteral("play-pause")) {
        return Command::PlayPause;
    }
    if (value == QStringLiteral("next")) {
        return Command::Next;
    }
    if (value == QStringLiteral("previous")) {
        return Command::Previous;
    }

    return std::nullopt;
}

QByteArray formatJsonEventBytes(const QString &reason,
                                const QString &playerId,
                                const plasma_bridge::MediaState &state,
                                const bool compactOutput)
{
    return QJsonDocument(plasma_bridge::toJsonEventObject(reason, playerId, state))
        .toJson(compactOutput ? QJsonDocument::Compact : QJsonDocument::Indented);
}

QString formatHumanEventText(const QString &reason,
                             const QString &playerId,
                             const plasma_bridge::MediaState &state)
{
    return plasma_bridge::formatHumanReadableEvent(reason, playerId, state);
}

QByteArray formatJsonControlBytes(const Command command,
                                  const control::MediaControlResult &result,
                                  const bool compactOutput)
{
    QJsonObject json;
    json[QStringLiteral("command")] = commandName(command);
    json[QStringLiteral("result")] = control::toJsonObject(result);
    return QJsonDocument(json).toJson(compactOutput ? QJsonDocument::Compact : QJsonDocument::Indented);
}

QString formatHumanControlText(const Command command, const control::MediaControlResult &result)
{
    QString output;
    QTextStream stream(&output);
    stream << "Command: " << commandName(command) << '\n';
    stream << control::formatHumanReadableResult(result);
    return output;
}

QString commandName(const Command command)
{
    switch (command) {
    case Command::Current:
        return QStringLiteral("current");
    case Command::Play:
        return QStringLiteral("play");
    case Command::Pause:
        return QStringLiteral("pause");
    case Command::PlayPause:
        return QStringLiteral("play-pause");
    case Command::Next:
        return QStringLiteral("next");
    case Command::Previous:
        return QStringLiteral("previous");
    }

    return QStringLiteral("current");
}

} // namespace plasma_bridge::tools::media_probe

#include "media_probe_runner.moc"
