#include "tests/support/test_support.h"
#include "tools/probes/audio_probe/audio_probe_runner.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("fake_audio_probe_cli"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Hermetic audio probe CLI helper."));
    plasma_bridge::tools::audio_probe::configureParser(parser);
    parser.addOption(QCommandLineOption(QStringLiteral("scenario"),
                                        QStringLiteral("One of: timeout, failure."),
                                        QStringLiteral("scenario"),
                                        QStringLiteral("timeout")));
    parser.process(app);

    QTextStream output(stdout);
    QTextStream error(stderr);

    plasma_bridge::tests::FakeAudioProbeSource source;
    plasma_bridge::tools::audio_probe::AudioProbeRunner runner(&source,
                                                               plasma_bridge::tools::audio_probe::optionsFromParser(parser),
                                                               &output,
                                                               &error,
                                                               20);
    QObject::connect(&runner, &plasma_bridge::tools::audio_probe::AudioProbeRunner::finished, &app, &QCoreApplication::exit);

    const QString scenario = parser.value(QStringLiteral("scenario"));
    if (scenario == QStringLiteral("failure")) {
        QTimer::singleShot(0, &source, [&source]() {
            source.emitConnectionFailure(QStringLiteral("Synthetic connection failure."));
        });
    }

    QTimer::singleShot(0, &runner, &plasma_bridge::tools::audio_probe::AudioProbeRunner::start);
    return app.exec();
}
