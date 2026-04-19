#include "adapters/audio/pulse_audio_volume_controller.h"
#include "tools/probes/audio_control_probe/audio_control_probe_runner.h"

#include <PulseAudioQt/Context>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("audio_control_probe"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Submit output sink volume changes through KF6PulseAudioQt."));
    plasma_bridge::tools::audio_control_probe::configureParser(parser);
    parser.process(app);

    if (parser.positionalArguments().size() != 3) {
        parser.showHelp(1);
    }

    PulseAudioQt::Context::setApplicationId(QStringLiteral("org.plasma-remote-toolbar.audio_control_probe"));

    QTextStream output(stdout);
    QTextStream error(stderr);

    const plasma_bridge::tools::audio_control_probe::ParseOptionsResult optionsResult =
        plasma_bridge::tools::audio_control_probe::parseOptions(parser);
    if (!optionsResult.ok) {
        error << optionsResult.errorMessage << Qt::endl;
        return 1;
    }

    plasma_bridge::audio::PulseAudioVolumeController controller;
    plasma_bridge::tools::audio_control_probe::PulseAudioSubmissionGate submissionGate;
    plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner runner(&controller,
                                                                              &submissionGate,
                                                                              *optionsResult.options,
                                                                              &output,
                                                                              &error);
    QObject::connect(&runner,
                     &plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner::finished,
                     &app,
                     &QCoreApplication::exit);
    QTimer::singleShot(0, &runner, &plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner::start);

    return app.exec();
}
