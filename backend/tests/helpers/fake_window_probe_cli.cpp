#include "tests/support/test_support.h"
#include "tools/probes/window_probe/window_probe_runner.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("fake_window_probe_cli"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Hermetic window probe CLI helper."));
    plasma_bridge::tools::window_probe::configureParser(parser);
    parser.addOption(QCommandLineOption(QStringLiteral("scenario"),
                                        QStringLiteral("One of: timeout, failure, list, no-active, activation-failure."),
                                        QStringLiteral("scenario"),
                                        QStringLiteral("timeout")));
    parser.process(app);

    QTextStream output(stdout);
    QTextStream error(stderr);

    const plasma_bridge::tools::window_probe::ParseOptionsResult optionsResult =
        plasma_bridge::tools::window_probe::parseOptions(parser);
    if (!optionsResult.ok) {
        error << optionsResult.errorMessage << Qt::endl;
        return 1;
    }

    plasma_bridge::tests::FakeWindowProbeSource source;
    plasma_bridge::tests::FakeWindowProbeBackendController backendController;
    plasma_bridge::tools::window_probe::WindowProbeRunner runner(&source,
                                                                 &backendController,
                                                                 *optionsResult.options,
                                                                 &output,
                                                                 &error);
    QObject::connect(&runner, &plasma_bridge::tools::window_probe::WindowProbeRunner::finished, &app, &QCoreApplication::exit);

    const QString scenario = parser.value(QStringLiteral("scenario"));
    if (scenario == QStringLiteral("failure")) {
        QTimer::singleShot(0, &source, [&source]() {
            source.emitConnectionFailure(QStringLiteral("Synthetic backend failure."));
        });
    } else if (scenario == QStringLiteral("list")) {
        QTimer::singleShot(0, &source, [&source]() {
            source.emitInitialSnapshotReady(plasma_bridge::tests::sampleWindowSnapshot());
        });
    } else if (scenario == QStringLiteral("no-active")) {
        QTimer::singleShot(0, &source, [&source]() {
            source.emitInitialSnapshotReady(plasma_bridge::tests::sampleWindowSnapshotWithoutActiveWindow());
        });
    } else if (scenario == QStringLiteral("activation-failure")) {
        plasma_bridge::control::WindowActivationResult result;
        result.status = plasma_bridge::control::WindowActivationStatus::WindowNotActivatable;
        backendController.setActivationResult(result);
        QTimer::singleShot(0, &source, [&source]() {
            source.emitInitialSnapshotReady(plasma_bridge::tests::sampleWindowSnapshot());
        });
    }

    QTimer::singleShot(0, &runner, &plasma_bridge::tools::window_probe::WindowProbeRunner::start);
    return app.exec();
}
