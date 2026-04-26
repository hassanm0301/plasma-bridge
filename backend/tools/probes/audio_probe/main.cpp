#include "plasma_bridge_build_config.h"
#include "tools/probes/audio_probe/audio_probe_runner.h"

#include <PulseAudioQt/Context>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("audio_probe"));
    QCoreApplication::setApplicationVersion(QStringLiteral(PLASMA_BRIDGE_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Observe KDE-visible audio sinks and sources through KF6PulseAudioQt."));
    plasma_bridge::tools::audio_probe::configureParser(parser);
    parser.process(app);

    PulseAudioQt::Context::setApplicationId(QStringLiteral(PLASMA_BRIDGE_AUDIO_PROBE_APP_ID));

    QTextStream output(stdout);
    QTextStream error(stderr);

    plasma_bridge::tools::audio_probe::PulseAudioProbeSource source;
    plasma_bridge::tools::audio_probe::AudioProbeRunner runner(&source,
                                                               plasma_bridge::tools::audio_probe::optionsFromParser(parser),
                                                               &output,
                                                               &error);
    QObject::connect(&runner, &plasma_bridge::tools::audio_probe::AudioProbeRunner::finished, &app, &QCoreApplication::exit);
    QTimer::singleShot(0, &runner, &plasma_bridge::tools::audio_probe::AudioProbeRunner::start);

    return app.exec();
}
