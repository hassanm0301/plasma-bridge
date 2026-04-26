#include "plasma_bridge_build_config.h"
#include "tools/probes/window_probe/window_probe_runner.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("window_probe"));
    QCoreApplication::setApplicationVersion(QStringLiteral(PLASMA_BRIDGE_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Inspect KDE Plasma window state through the window_probe KWin script backend."));
    plasma_bridge::tools::window_probe::configureParser(parser);
    parser.process(app);

    QTextStream output(stdout);
    QTextStream error(stderr);

    const plasma_bridge::tools::window_probe::ParseOptionsResult optionsResult =
        plasma_bridge::tools::window_probe::parseOptions(parser);
    if (!optionsResult.ok) {
        error << optionsResult.errorMessage << Qt::endl;
        return 1;
    }

    plasma_bridge::tools::window_probe::KWinScriptWindowProbeSource source;
    plasma_bridge::tools::window_probe::KWinScriptWindowProbeBackendController backendController;
    plasma_bridge::tools::window_probe::WindowProbeRunner runner(&source,
                                                                 &backendController,
                                                                 *optionsResult.options,
                                                                 &output,
                                                                 &error);
    QObject::connect(&runner, &plasma_bridge::tools::window_probe::WindowProbeRunner::finished, &app, &QCoreApplication::exit);
    QTimer::singleShot(0, &runner, &plasma_bridge::tools::window_probe::WindowProbeRunner::start);

    return app.exec();
}
