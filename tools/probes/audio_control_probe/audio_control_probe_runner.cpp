#include "tools/probes/audio_control_probe/audio_control_probe_runner.h"

#include "adapters/audio/pulse_audio_volume_controller.h"

#include <PulseAudioQt/Context>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QTextStream>
#include <QTimer>

#include <limits>

namespace plasma_bridge::tools::audio_control_probe
{

class PulseAudioSubmissionGate::Impl final : public QObject
{
    Q_OBJECT

public:
    explicit Impl(PulseAudioSubmissionGate *owner)
        : QObject(owner)
        , m_context(PulseAudioQt::Context::instance())
    {
        if (m_context != nullptr) {
            connect(m_context, &PulseAudioQt::Context::stateChanged, owner, &AudioControlSubmissionGate::submissionConditionChanged);
        }
    }

    PulseAudioQt::Context *m_context = nullptr;
};

PulseAudioSubmissionGate::PulseAudioSubmissionGate(QObject *parent)
    : AudioControlSubmissionGate(parent)
    , m_impl(new Impl(this))
{
}

bool PulseAudioSubmissionGate::shouldSubmitRequest() const
{
    if (m_impl->m_context == nullptr) {
        return true;
    }

    using ContextState = PulseAudioQt::Context::State;
    const ContextState state = m_impl->m_context->state();
    return state == ContextState::Ready || state == ContextState::Failed || state == ContextState::Terminated;
}

AudioControlProbeRunner::AudioControlProbeRunner(control::AudioVolumeController *controller,
                                                 AudioControlSubmissionGate *submissionGate,
                                                 const AudioControlProbeOptions &options,
                                                 QTextStream *output,
                                                 QTextStream *error,
                                                 QObject *parent)
    : QObject(parent)
    , m_controller(controller)
    , m_submissionGate(submissionGate)
    , m_options(options)
    , m_output(output)
    , m_error(error)
    , m_startupTimer(new QTimer(this))
{
    m_startupTimer->setSingleShot(true);
    connect(m_startupTimer, &QTimer::timeout, this, &AudioControlProbeRunner::submitRequest);

    if (m_submissionGate != nullptr) {
        connect(m_submissionGate, &AudioControlSubmissionGate::submissionConditionChanged, this, [this]() {
            if (m_submissionGate != nullptr && m_submissionGate->shouldSubmitRequest()) {
                submitRequest();
            }
        });
    }
}

void AudioControlProbeRunner::start()
{
    if (m_finished || m_controller == nullptr) {
        if (!m_finished && m_error != nullptr) {
            *m_error << "Audio control probe controller is unavailable." << Qt::endl;
        }
        finish(1);
        return;
    }

    if (m_submissionGate == nullptr || m_submissionGate->shouldSubmitRequest()) {
        QTimer::singleShot(0, this, &AudioControlProbeRunner::submitRequest);
        return;
    }

    m_startupTimer->start(m_options.timeoutMs);
}

void AudioControlProbeRunner::finish(const int exitCode)
{
    if (m_finished) {
        return;
    }

    m_finished = true;
    m_startupTimer->stop();
    emit finished(exitCode);
}

void AudioControlProbeRunner::submitRequest()
{
    if (m_finished || m_controller == nullptr) {
        return;
    }

    control::VolumeChangeResult result;
    switch (m_options.command) {
    case Command::Set:
        result = m_controller->setVolume(m_options.sinkId, m_options.requestedValue);
        break;
    case Command::Increment:
        result = m_controller->incrementVolume(m_options.sinkId, m_options.requestedValue);
        break;
    case Command::Decrement:
        result = m_controller->decrementVolume(m_options.sinkId, m_options.requestedValue);
        break;
    }

    if (m_output != nullptr) {
        if (m_options.jsonOutput) {
            *m_output << formatJsonResultBytes(result) << Qt::endl;
        } else {
            *m_output << formatHumanResultText(result) << Qt::endl;
        }
    }

    finish(exitCodeForResult(result));
}

void configureParser(QCommandLineParser &parser)
{
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption(QStringLiteral("json"),
                                        QStringLiteral("Print machine-readable JSON output.")));
    parser.addOption(QCommandLineOption(QStringLiteral("timeout-ms"),
                                        QStringLiteral("Maximum time to wait for PulseAudioQt readiness."),
                                        QStringLiteral("milliseconds"),
                                        QString::number(kDefaultStartupTimeoutMs)));
    parser.addPositionalArgument(QStringLiteral("command"),
                                 QStringLiteral("One of: set, increment, decrement."));
    parser.addPositionalArgument(QStringLiteral("sink-id"),
                                 QStringLiteral("Canonical output sink id."));
    parser.addPositionalArgument(QStringLiteral("value"),
                                 QStringLiteral("Normalized target or delta value."));
}

ParseOptionsResult parseOptions(const QCommandLineParser &parser)
{
    ParseOptionsResult result;
    const QStringList arguments = parser.positionalArguments();
    if (arguments.size() != 3) {
        result.errorMessage = QStringLiteral("Expected command, sink-id, and value positional arguments.");
        return result;
    }

    const std::optional<Command> command = parseCommand(arguments.at(0));
    if (!command.has_value()) {
        result.errorMessage = QStringLiteral("Unsupported command: %1").arg(arguments.at(0));
        return result;
    }

    const std::optional<double> requestedValue = parseRequestedValue(arguments.at(2));
    if (!requestedValue.has_value()) {
        result.errorMessage = QStringLiteral("Value must be numeric or one of: nan, inf, -inf.");
        return result;
    }

    bool ok = false;
    const int timeoutMs = parser.value(QStringLiteral("timeout-ms")).toInt(&ok);
    if (!ok || timeoutMs < 0) {
        result.errorMessage = QStringLiteral("timeout-ms must be a non-negative integer.");
        return result;
    }

    AudioControlProbeOptions options;
    options.command = *command;
    options.sinkId = arguments.at(1);
    options.requestedValue = *requestedValue;
    options.timeoutMs = timeoutMs;
    options.jsonOutput = parser.isSet(QStringLiteral("json"));

    result.ok = true;
    result.options = options;
    return result;
}

std::optional<Command> parseCommand(const QString &value)
{
    if (value == QStringLiteral("set")) {
        return Command::Set;
    }
    if (value == QStringLiteral("increment")) {
        return Command::Increment;
    }
    if (value == QStringLiteral("decrement")) {
        return Command::Decrement;
    }

    return std::nullopt;
}

std::optional<double> parseRequestedValue(const QString &value)
{
    const QString trimmedValue = value.trimmed();
    const QString lowerValue = trimmedValue.toLower();

    if (lowerValue == QStringLiteral("nan")) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (lowerValue == QStringLiteral("inf") || lowerValue == QStringLiteral("+inf")
        || lowerValue == QStringLiteral("infinity") || lowerValue == QStringLiteral("+infinity")) {
        return std::numeric_limits<double>::infinity();
    }
    if (lowerValue == QStringLiteral("-inf") || lowerValue == QStringLiteral("-infinity")) {
        return -std::numeric_limits<double>::infinity();
    }

    bool ok = false;
    const double parsedValue = trimmedValue.toDouble(&ok);
    if (!ok) {
        return std::nullopt;
    }

    return parsedValue;
}

QByteArray formatJsonResultBytes(const control::VolumeChangeResult &result)
{
    return QJsonDocument(control::toJsonObject(result)).toJson(QJsonDocument::Indented);
}

QString formatHumanResultText(const control::VolumeChangeResult &result)
{
    return control::formatHumanReadableResult(result);
}

int exitCodeForResult(const control::VolumeChangeResult &result)
{
    return result.status == control::VolumeChangeStatus::Accepted ? 0 : 1;
}

} // namespace plasma_bridge::tools::audio_control_probe

#include "audio_control_probe_runner.moc"
