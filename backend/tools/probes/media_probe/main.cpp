#include "plasma_bridge_build_config.h"
#include "adapters/media/mpris_media_controller.h"
#include "adapters/media/mpris_media_observer.h"
#include "state/media_state_store.h"
#include "tools/probes/media_probe/media_probe_runner.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("media_probe"));
    QCoreApplication::setApplicationVersion(QStringLiteral(PLASMA_BRIDGE_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Inspect and control the current MPRIS media session."));
    plasma_bridge::tools::media_probe::configureParser(parser);
    parser.process(app);

    QTextStream output(stdout);
    QTextStream error(stderr);

    const plasma_bridge::tools::media_probe::ParseOptionsResult optionsResult =
        plasma_bridge::tools::media_probe::parseOptions(parser);
    if (!optionsResult.ok) {
        error << optionsResult.errorMessage << Qt::endl;
        return 1;
    }

    plasma_bridge::media::MprisMediaObserver observer;
    plasma_bridge::state::MediaStateStore stateStore;
    stateStore.attachObserver(&observer);
    plasma_bridge::tools::media_probe::MprisMediaProbeSource source(&observer);
    plasma_bridge::media::MprisMediaController controller(&stateStore);
    plasma_bridge::tools::media_probe::MediaProbeRunner runner(&source,
                                                               &controller,
                                                               *optionsResult.options,
                                                               &output,
                                                               &error);
    QObject::connect(&runner, &plasma_bridge::tools::media_probe::MediaProbeRunner::finished, &app, &QCoreApplication::exit);
    QTimer::singleShot(0, &runner, &plasma_bridge::tools::media_probe::MediaProbeRunner::start);

    return app.exec();
}
