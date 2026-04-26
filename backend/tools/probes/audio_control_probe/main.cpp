#include "plasma_bridge_build_config.h"
#include "adapters/audio/pulse_audio_device_controller.h"
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
    QCoreApplication::setApplicationVersion(QStringLiteral(PLASMA_BRIDGE_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Submit audio control requests through KF6PulseAudioQt."));
    plasma_bridge::tools::audio_control_probe::configureParser(parser);
    parser.process(app);

    PulseAudioQt::Context::setApplicationId(QStringLiteral(PLASMA_BRIDGE_AUDIO_CONTROL_PROBE_APP_ID));

    QTextStream output(stdout);
    QTextStream error(stderr);

    const plasma_bridge::tools::audio_control_probe::ParseOptionsResult optionsResult =
        plasma_bridge::tools::audio_control_probe::parseOptions(parser);
    if (!optionsResult.ok) {
        error << optionsResult.errorMessage << Qt::endl;
        return 1;
    }

    plasma_bridge::audio::PulseAudioVolumeController controller;
    plasma_bridge::audio::PulseAudioDeviceController deviceController;
    plasma_bridge::tools::audio_control_probe::PulseAudioSubmissionGate submissionGate;
    plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner runner(&controller,
                                                                              &deviceController,
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
