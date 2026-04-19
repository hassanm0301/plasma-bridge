#include "tools/probes/audio_control_probe/audio_control_probe_runner.h"

#include <PulseAudioQt/Context>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QTextStream>
#include <QTimer>

#include <limits>

namespace plasma_bridge::tools::audio_control_probe
{
namespace
{

QString commandName(const Command command)
{
    switch (command) {
    case Command::SetVolume:
        return QStringLiteral("set");
    case Command::IncrementVolume:
        return QStringLiteral("increment");
    case Command::DecrementVolume:
        return QStringLiteral("decrement");
    case Command::SetDefaultSink:
        return QStringLiteral("set-default-sink");
    case Command::SetDefaultSource:
        return QStringLiteral("set-default-source");
    case Command::SetSinkMute:
        return QStringLiteral("set-sink-mute");
    case Command::SetSourceMute:
        return QStringLiteral("set-source-mute");
    }

    return QStringLiteral("unknown");
}

bool commandUsesVolumeValue(const Command command)
{
    return command == Command::SetVolume
        || command == Command::IncrementVolume
        || command == Command::DecrementVolume;
}

bool commandUsesMutedValue(const Command command)
{
    return command == Command::SetSinkMute || command == Command::SetSourceMute;
}

bool commandUsesDeviceController(const Command command)
{
    return command == Command::SetDefaultSink
        || command == Command::SetDefaultSource
        || commandUsesMutedValue(command);
}

QString positionalArgumentsError(const Command command)
{
    if (commandUsesVolumeValue(command)) {
        return QStringLiteral("Command %1 expects device-id and value positional arguments.").arg(commandName(command));
    }
    if (commandUsesMutedValue(command)) {
        return QStringLiteral("Command %1 expects device-id and muted positional arguments.").arg(commandName(command));
    }

    return QStringLiteral("Command %1 expects a device-id positional argument.").arg(commandName(command));
}

} // namespace

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

AudioControlProbeRunner::AudioControlProbeRunner(control::AudioVolumeController *volumeController,
                                                 control::AudioDeviceController *deviceController,
                                                 AudioControlSubmissionGate *submissionGate,
                                                 const AudioControlProbeOptions &options,
                                                 QTextStream *output,
                                                 QTextStream *error,
                                                 QObject *parent)
    : QObject(parent)
    , m_volumeController(volumeController)
    , m_deviceController(deviceController)
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

bool AudioControlProbeRunner::hasRequiredController() const
{
    if (commandUsesDeviceController(m_options.command)) {
        return m_deviceController != nullptr;
    }

    return m_volumeController != nullptr;
}

QString AudioControlProbeRunner::unavailableControllerMessage() const
{
    if (commandUsesDeviceController(m_options.command)) {
        return QStringLiteral("Audio control probe device controller is unavailable.");
    }

    return QStringLiteral("Audio control probe volume controller is unavailable.");
}

void AudioControlProbeRunner::start()
{
    if (m_finished || !hasRequiredController()) {
        if (!m_finished && m_error != nullptr) {
            *m_error << unavailableControllerMessage() << Qt::endl;
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
    if (m_finished || !hasRequiredController()) {
        return;
    }

    const auto writeResult = [this](const auto &result) {
        if (m_output != nullptr) {
            if (m_options.jsonOutput) {
                *m_output << formatJsonResultBytes(result) << Qt::endl;
            } else {
                *m_output << formatHumanResultText(result) << Qt::endl;
            }
        }

        finish(exitCodeForResult(result));
    };

    switch (m_options.command) {
    case Command::SetVolume:
        writeResult(m_volumeController->setVolume(m_options.deviceId, *m_options.requestedValue));
        return;
    case Command::IncrementVolume:
        writeResult(m_volumeController->incrementVolume(m_options.deviceId, *m_options.requestedValue));
        return;
    case Command::DecrementVolume:
        writeResult(m_volumeController->decrementVolume(m_options.deviceId, *m_options.requestedValue));
        return;
    case Command::SetDefaultSink:
        writeResult(m_deviceController->setDefaultSink(m_options.deviceId));
        return;
    case Command::SetDefaultSource:
        writeResult(m_deviceController->setDefaultSource(m_options.deviceId));
        return;
    case Command::SetSinkMute:
        writeResult(m_deviceController->setSinkMuted(m_options.deviceId, *m_options.requestedMuted));
        return;
    case Command::SetSourceMute:
        writeResult(m_deviceController->setSourceMuted(m_options.deviceId, *m_options.requestedMuted));
        return;
    }
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
                                 QStringLiteral("One of: set, increment, decrement, set-default-sink, set-default-source, set-sink-mute, set-source-mute."));
    parser.addPositionalArgument(QStringLiteral("device-id"),
                                 QStringLiteral("Canonical sink or source id, depending on the command."));
    parser.addPositionalArgument(QStringLiteral("value"),
                                 QStringLiteral("Normalized value for volume commands or true/false for mute commands when required."));
}

ParseOptionsResult parseOptions(const QCommandLineParser &parser)
{
    ParseOptionsResult result;
    const QStringList arguments = parser.positionalArguments();
    if (arguments.isEmpty()) {
        result.errorMessage = QStringLiteral("Expected a command positional argument.");
        return result;
    }

    const std::optional<Command> command = parseCommand(arguments.at(0));
    if (!command.has_value()) {
        result.errorMessage = QStringLiteral("Unsupported command: %1").arg(arguments.at(0));
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
    options.timeoutMs = timeoutMs;
    options.jsonOutput = parser.isSet(QStringLiteral("json"));

    if (commandUsesVolumeValue(*command)) {
        if (arguments.size() != 3) {
            result.errorMessage = positionalArgumentsError(*command);
            return result;
        }

        const std::optional<double> requestedValue = parseRequestedValue(arguments.at(2));
        if (!requestedValue.has_value()) {
            result.errorMessage = QStringLiteral("Value must be numeric or one of: nan, inf, -inf.");
            return result;
        }

        options.deviceId = arguments.at(1);
        options.requestedValue = *requestedValue;
    } else if (commandUsesMutedValue(*command)) {
        if (arguments.size() != 3) {
            result.errorMessage = positionalArgumentsError(*command);
            return result;
        }

        const std::optional<bool> requestedMuted = parseRequestedMuted(arguments.at(2));
        if (!requestedMuted.has_value()) {
            result.errorMessage = QStringLiteral("Muted must be true or false.");
            return result;
        }

        options.deviceId = arguments.at(1);
        options.requestedMuted = *requestedMuted;
    } else {
        if (arguments.size() != 2) {
            result.errorMessage = positionalArgumentsError(*command);
            return result;
        }

        options.deviceId = arguments.at(1);
    }

    result.ok = true;
    result.options = options;
    return result;
}

std::optional<Command> parseCommand(const QString &value)
{
    if (value == QStringLiteral("set")) {
        return Command::SetVolume;
    }
    if (value == QStringLiteral("increment")) {
        return Command::IncrementVolume;
    }
    if (value == QStringLiteral("decrement")) {
        return Command::DecrementVolume;
    }
    if (value == QStringLiteral("set-default-sink")) {
        return Command::SetDefaultSink;
    }
    if (value == QStringLiteral("set-default-source")) {
        return Command::SetDefaultSource;
    }
    if (value == QStringLiteral("set-sink-mute")) {
        return Command::SetSinkMute;
    }
    if (value == QStringLiteral("set-source-mute")) {
        return Command::SetSourceMute;
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

std::optional<bool> parseRequestedMuted(const QString &value)
{
    const QString lowerValue = value.trimmed().toLower();
    if (lowerValue == QStringLiteral("true")) {
        return true;
    }
    if (lowerValue == QStringLiteral("false")) {
        return false;
    }

    return std::nullopt;
}

QByteArray formatJsonResultBytes(const control::VolumeChangeResult &result)
{
    return QJsonDocument(control::toJsonObject(result)).toJson(QJsonDocument::Indented);
}

QByteArray formatJsonResultBytes(const control::DefaultDeviceChangeResult &result)
{
    return QJsonDocument(control::toJsonObject(result)).toJson(QJsonDocument::Indented);
}

QByteArray formatJsonResultBytes(const control::MuteChangeResult &result)
{
    return QJsonDocument(control::toJsonObject(result)).toJson(QJsonDocument::Indented);
}

QString formatHumanResultText(const control::VolumeChangeResult &result)
{
    return control::formatHumanReadableResult(result);
}

QString formatHumanResultText(const control::DefaultDeviceChangeResult &result)
{
    return control::formatHumanReadableResult(result);
}

QString formatHumanResultText(const control::MuteChangeResult &result)
{
    return control::formatHumanReadableResult(result);
}

int exitCodeForResult(const control::VolumeChangeResult &result)
{
    return result.status == control::VolumeChangeStatus::Accepted ? 0 : 1;
}

int exitCodeForResult(const control::DefaultDeviceChangeResult &result)
{
    return result.status == control::AudioDeviceChangeStatus::Accepted ? 0 : 1;
}

int exitCodeForResult(const control::MuteChangeResult &result)
{
    return result.status == control::AudioDeviceChangeStatus::Accepted ? 0 : 1;
}

} // namespace plasma_bridge::tools::audio_control_probe

#include "audio_control_probe_runner.moc"
