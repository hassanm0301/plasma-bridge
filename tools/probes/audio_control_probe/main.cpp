#include "adapters/audio/pulse_audio_volume_controller.h"
#include "control/audio_volume_controller.h"

#include <PulseAudioQt/Context>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QTextStream>
#include <QTimer>

#include <limits>
#include <optional>

namespace
{

constexpr int kStartupTimeoutMs = 5000;

enum class Command {
    Set,
    Increment,
    Decrement,
};

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

void printResult(const plasma_bridge::control::VolumeChangeResult &result, const bool jsonOutput)
{
    QTextStream output(stdout);

    if (jsonOutput) {
        const QJsonDocument document(plasma_bridge::control::toJsonObject(result));
        output << document.toJson(QJsonDocument::Indented) << Qt::endl;
        return;
    }

    output << plasma_bridge::control::formatHumanReadableResult(result) << Qt::endl;
}

int exitCodeForResult(const plasma_bridge::control::VolumeChangeResult &result)
{
    return result.status == plasma_bridge::control::VolumeChangeStatus::Accepted ? 0 : 1;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("audio_control_probe"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Submit output sink volume changes through KF6PulseAudioQt."));
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption jsonOption(QStringLiteral("json"),
                                  QStringLiteral("Print machine-readable JSON output."));
    QCommandLineOption timeoutOption(QStringLiteral("timeout-ms"),
                                     QStringLiteral("Maximum time to wait for PulseAudioQt readiness."),
                                     QStringLiteral("milliseconds"),
                                     QString::number(kStartupTimeoutMs));

    parser.addOption(jsonOption);
    parser.addOption(timeoutOption);
    parser.addPositionalArgument(QStringLiteral("command"),
                                 QStringLiteral("One of: set, increment, decrement."));
    parser.addPositionalArgument(QStringLiteral("sink-id"),
                                 QStringLiteral("Canonical output sink id."));
    parser.addPositionalArgument(QStringLiteral("value"),
                                 QStringLiteral("Normalized target or delta value."));
    parser.process(app);

    const QStringList arguments = parser.positionalArguments();
    if (arguments.size() != 3) {
        parser.showHelp(1);
    }

    const std::optional<Command> command = parseCommand(arguments.at(0));
    if (!command.has_value()) {
        QTextStream error(stderr);
        error << "Unsupported command: " << arguments.at(0) << Qt::endl;
        return 1;
    }

    const std::optional<double> requestedValue = parseRequestedValue(arguments.at(2));
    if (!requestedValue.has_value()) {
        QTextStream error(stderr);
        error << "Value must be numeric or one of: nan, inf, -inf." << Qt::endl;
        return 1;
    }

    bool ok = false;
    const int timeoutMs = parser.value(timeoutOption).toInt(&ok);
    if (!ok || timeoutMs < 0) {
        QTextStream error(stderr);
        error << "timeout-ms must be a non-negative integer." << Qt::endl;
        return 1;
    }

    const bool jsonOutput = parser.isSet(jsonOption);
    const QString sinkId = arguments.at(1);

    PulseAudioQt::Context::setApplicationId(QStringLiteral("org.plasma-remote-toolbar.audio_control_probe"));

    plasma_bridge::audio::PulseAudioVolumeController controller;
    QTimer startupTimer;
    startupTimer.setSingleShot(true);

    bool completed = false;
    const auto submitRequest = [&]() {
        if (completed) {
            return;
        }
        completed = true;
        startupTimer.stop();

        plasma_bridge::control::VolumeChangeResult result;
        switch (*command) {
        case Command::Set:
            result = controller.setVolume(sinkId, *requestedValue);
            break;
        case Command::Increment:
            result = controller.incrementVolume(sinkId, *requestedValue);
            break;
        case Command::Decrement:
            result = controller.decrementVolume(sinkId, *requestedValue);
            break;
        }

        printResult(result, jsonOutput);
        app.exit(exitCodeForResult(result));
    };

    PulseAudioQt::Context *context = PulseAudioQt::Context::instance();
    if (context == nullptr || context->state() == PulseAudioQt::Context::State::Ready) {
        QTimer::singleShot(0, &app, submitRequest);
        return app.exec();
    }

    QObject::connect(&startupTimer, &QTimer::timeout, &app, submitRequest);
    QObject::connect(context, &PulseAudioQt::Context::stateChanged, &app, [context, &submitRequest]() {
        const PulseAudioQt::Context::State state = context->state();
        if (state == PulseAudioQt::Context::State::Ready
            || state == PulseAudioQt::Context::State::Failed
            || state == PulseAudioQt::Context::State::Terminated) {
            submitRequest();
        }
    });

    startupTimer.start(timeoutMs);
    return app.exec();
}
