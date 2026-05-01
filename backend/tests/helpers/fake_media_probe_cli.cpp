#include "tests/support/test_support.h"
#include "tools/probes/media_probe/media_probe_runner.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

namespace
{

class FakeMediaProbeSource final : public plasma_bridge::tools::media_probe::MediaProbeSource
{
    Q_OBJECT

public:
    using MediaProbeSource::MediaProbeSource;

    void start() override
    {
    }

    const plasma_bridge::MediaState &currentState() const override
    {
        return m_state;
    }

    bool hasInitialState() const override
    {
        return m_ready;
    }

    void emitInitial(const plasma_bridge::MediaState &state)
    {
        m_state = state;
        m_ready = true;
        emit initialStateReady();
    }

    void emitFailure(const QString &message)
    {
        emit connectionFailed(message);
    }

private:
    plasma_bridge::MediaState m_state;
    bool m_ready = false;
};

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("fake_media_probe_cli"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Hermetic media probe CLI helper."));
    plasma_bridge::tools::media_probe::configureParser(parser);
    parser.addOption(QCommandLineOption(QStringLiteral("scenario"),
                                        QStringLiteral("One of: timeout, failure, current, no-player, control-failure."),
                                        QStringLiteral("scenario"),
                                        QStringLiteral("timeout")));
    parser.process(app);

    QTextStream output(stdout);
    QTextStream error(stderr);

    const plasma_bridge::tools::media_probe::ParseOptionsResult optionsResult =
        plasma_bridge::tools::media_probe::parseOptions(parser);
    if (!optionsResult.ok) {
        error << optionsResult.errorMessage << Qt::endl;
        return 1;
    }

    FakeMediaProbeSource source;
    plasma_bridge::tests::FakeMediaController controller;
    plasma_bridge::tools::media_probe::MediaProbeRunner runner(&source,
                                                               &controller,
                                                               *optionsResult.options,
                                                               &output,
                                                               &error);
    QObject::connect(&runner, &plasma_bridge::tools::media_probe::MediaProbeRunner::finished, &app, &QCoreApplication::exit);

    const QString scenario = parser.value(QStringLiteral("scenario"));
    if (scenario == QStringLiteral("failure")) {
        QTimer::singleShot(0, &source, [&source]() {
            source.emitFailure(QStringLiteral("Synthetic media failure."));
        });
    } else if (scenario == QStringLiteral("current")) {
        QTimer::singleShot(0, &source, [&source]() {
            source.emitInitial(plasma_bridge::tests::sampleMediaState());
        });
    } else if (scenario == QStringLiteral("no-player")) {
        QTimer::singleShot(0, &source, [&source]() {
            source.emitInitial(plasma_bridge::tests::sampleMediaStateWithoutPlayer());
        });
    } else if (scenario == QStringLiteral("control-failure")) {
        plasma_bridge::control::MediaControlResult result;
        result.status = plasma_bridge::control::MediaControlStatus::ActionNotSupported;
        result.playerId = QStringLiteral("org.mpris.MediaPlayer2.spotify");
        controller.setResult(plasma_bridge::control::MediaControlAction::PlayPause, result);
        QTimer::singleShot(0, &source, [&source]() {
            source.emitInitial(plasma_bridge::tests::sampleMediaState());
        });
    }

    QTimer::singleShot(0, &runner, &plasma_bridge::tools::media_probe::MediaProbeRunner::start);
    return app.exec();
}

#include "fake_media_probe_cli.moc"
