#include "tools/probes/audio_probe/audio_probe_runner.h"

#include <PulseAudioQt/Context>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("audio_probe"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Observe KDE-visible audio sinks through KF6PulseAudioQt."));
    plasma_bridge::tools::audio_probe::configureParser(parser);
    parser.process(app);

    PulseAudioQt::Context::setApplicationId(QStringLiteral("org.plasma-remote-toolbar.audio_probe"));

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
