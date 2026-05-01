#include "tests/support/test_support.h"
#include "tools/probes/media_probe/media_probe_runner.h"

#include <QCommandLineParser>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>

namespace
{

class FakeMediaProbeSource final : public plasma_bridge::tools::media_probe::MediaProbeSource
{
    Q_OBJECT

public:
    using MediaProbeSource::MediaProbeSource;

    void start() override
    {
        ++m_startCount;
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

    void emitChange(const QString &reason, const QString &playerId, const plasma_bridge::MediaState &state)
    {
        m_state = state;
        m_ready = true;
        emit mediaStateChanged(reason, playerId);
    }

    void emitFailure(const QString &message)
    {
        emit connectionFailed(message);
    }

    int m_startCount = 0;
    plasma_bridge::MediaState m_state;
    bool m_ready = false;
};

} // namespace

class MediaProbeRunnerTest : public QObject
{
    Q_OBJECT

private slots:
    void parseOptionsHandlesCommandsAndFailures();
    void runnerPrintsIndentedJsonForCurrentPlayer();
    void runnerPrintsWatchEvents();
    void runnerExecutesControlCommand();
    void runnerTimesOutBeforeInitialState();
    void runnerFailsWhenConnectionBreaksBeforeReady();
};

void MediaProbeRunnerTest::parseOptionsHandlesCommandsAndFailures()
{
    using namespace plasma_bridge::tools::media_probe;

    QCommandLineParser parser;
    configureParser(parser);
    parser.process(QStringList{QStringLiteral("media_probe"),
                               QStringLiteral("--watch"),
                               QStringLiteral("--json"),
                               QStringLiteral("--timeout-ms"),
                               QStringLiteral("25"),
                               QStringLiteral("current")});
    const ParseOptionsResult current = parseOptions(parser);
    QVERIFY(current.ok);
    QCOMPARE(current.options->command, Command::Current);
    QCOMPARE(current.options->watchMode, true);
    QCOMPARE(current.options->jsonOutput, true);
    QCOMPARE(current.options->timeoutMs, 25);

    QCommandLineParser playParser;
    configureParser(playParser);
    playParser.process(QStringList{QStringLiteral("media_probe"), QStringLiteral("play")});
    const ParseOptionsResult play = parseOptions(playParser);
    QVERIFY(play.ok);
    QCOMPARE(play.options->command, Command::Play);

    QCommandLineParser invalidParser;
    configureParser(invalidParser);
    invalidParser.process(QStringList{QStringLiteral("media_probe"), QStringLiteral("seek")});
    const ParseOptionsResult invalid = parseOptions(invalidParser);
    QVERIFY(!invalid.ok);
    QVERIFY(invalid.errorMessage.contains(QStringLiteral("Unsupported command")));

    QCommandLineParser watchParser;
    configureParser(watchParser);
    watchParser.process(QStringList{QStringLiteral("media_probe"), QStringLiteral("--watch"), QStringLiteral("play")});
    const ParseOptionsResult invalidWatch = parseOptions(watchParser);
    QVERIFY(!invalidWatch.ok);
    QVERIFY(invalidWatch.errorMessage.contains(QStringLiteral("--watch")));
}

void MediaProbeRunnerTest::runnerPrintsIndentedJsonForCurrentPlayer()
{
    FakeMediaProbeSource source;
    plasma_bridge::tests::FakeMediaController controller;
    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::media_probe::MediaProbeOptions options;
    options.jsonOutput = true;

    plasma_bridge::tools::media_probe::MediaProbeRunner runner(&source, &controller, options, &output, &error);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::media_probe::MediaProbeRunner::finished);

    QTimer::singleShot(0, &source, [&source]() {
        source.emitInitial(plasma_bridge::tests::sampleMediaState());
    });

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 0);
    QCOMPARE(source.m_startCount, 1);
    QVERIFY(outputText.contains(QStringLiteral("\"event\": \"mediaState\"")));
    QVERIFY(outputText.contains(QStringLiteral("\"reason\": \"initial\"")));
    QVERIFY(outputText.contains(QStringLiteral("\"playerId\": \"org.mpris.MediaPlayer2.spotify\"")));
    QVERIFY(errorText.isEmpty());
}

void MediaProbeRunnerTest::runnerPrintsWatchEvents()
{
    FakeMediaProbeSource source;
    plasma_bridge::tests::FakeMediaController controller;
    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::media_probe::MediaProbeOptions options;
    options.command = plasma_bridge::tools::media_probe::Command::Current;
    options.watchMode = true;

    plasma_bridge::tools::media_probe::MediaProbeRunner runner(&source, &controller, options, &output, &error);

    QTimer::singleShot(0, &source, [&source]() {
        source.emitInitial(plasma_bridge::tests::sampleMediaState());
        source.emitChange(QStringLiteral("player-updated"),
                          QStringLiteral("org.mpris.MediaPlayer2.firefox.instance1234"),
                          plasma_bridge::tests::sampleMediaStateWithoutPlayer());
    });

    runner.start();

    QTRY_VERIFY(outputText.contains(QStringLiteral("Reason: initial")));
    QTRY_VERIFY(outputText.contains(QStringLiteral("Reason: player-updated")));
    QVERIFY(outputText.contains(QStringLiteral("Current Player: (none)")));
    QVERIFY(errorText.isEmpty());
}

void MediaProbeRunnerTest::runnerExecutesControlCommand()
{
    FakeMediaProbeSource source;
    plasma_bridge::tests::FakeMediaController controller;
    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::control::MediaControlResult result;
    result.status = plasma_bridge::control::MediaControlStatus::Accepted;
    result.playerId = QStringLiteral("org.mpris.MediaPlayer2.spotify");
    controller.setResult(plasma_bridge::control::MediaControlAction::PlayPause, result);

    plasma_bridge::tools::media_probe::MediaProbeOptions options;
    options.command = plasma_bridge::tools::media_probe::Command::PlayPause;
    options.jsonOutput = true;

    plasma_bridge::tools::media_probe::MediaProbeRunner runner(&source, &controller, options, &output, &error);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::media_probe::MediaProbeRunner::finished);

    QTimer::singleShot(0, &source, [&source]() {
        source.emitInitial(plasma_bridge::tests::sampleMediaState());
    });

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 0);
    QCOMPARE(controller.lastOperation(), plasma_bridge::control::MediaControlAction::PlayPause);
    QVERIFY(outputText.contains(QStringLiteral("\"command\": \"play-pause\"")));
    QVERIFY(outputText.contains(QStringLiteral("\"status\": \"accepted\"")));
    QVERIFY(errorText.isEmpty());
}

void MediaProbeRunnerTest::runnerTimesOutBeforeInitialState()
{
    FakeMediaProbeSource source;
    plasma_bridge::tests::FakeMediaController controller;
    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::media_probe::MediaProbeOptions options;
    options.timeoutMs = 10;

    plasma_bridge::tools::media_probe::MediaProbeRunner runner(&source, &controller, options, &output, &error);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::media_probe::MediaProbeRunner::finished);

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 1);
    QVERIFY(errorText.contains(QStringLiteral("Timed out waiting for MPRIS media state.")));
}

void MediaProbeRunnerTest::runnerFailsWhenConnectionBreaksBeforeReady()
{
    FakeMediaProbeSource source;
    plasma_bridge::tests::FakeMediaController controller;
    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::media_probe::MediaProbeRunner runner(&source, &controller, {}, &output, &error);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::media_probe::MediaProbeRunner::finished);

    QTimer::singleShot(0, &source, [&source]() {
        source.emitFailure(QStringLiteral("Synthetic media failure."));
    });

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 1);
    QVERIFY(errorText.contains(QStringLiteral("Synthetic media failure.")));
}

QTEST_GUILESS_MAIN(MediaProbeRunnerTest)

#include "test_media_probe_runner.moc"
