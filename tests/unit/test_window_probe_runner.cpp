#include "tests/support/test_support.h"
#include "tools/probes/window_probe/window_probe_runner.h"

#include <QCommandLineParser>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>

class WindowProbeRunnerTest : public QObject
{
    Q_OBJECT

private slots:
    void parseOptionsHandlesCommandsAndFailures();
    void runnerPrintsListJsonAndExitsInOneShotMode();
    void runnerPrintsActiveJsonWithNullWindow();
    void runnerTimesOutBeforeInitialSnapshot();
    void runnerFailsWhenConnectionBreaksBeforeReady();
};

void WindowProbeRunnerTest::parseOptionsHandlesCommandsAndFailures()
{
    using namespace plasma_bridge::tools::window_probe;

    QCommandLineParser parser;
    configureParser(parser);
    parser.process(QStringList{QStringLiteral("window_probe"),
                               QStringLiteral("--json"),
                               QStringLiteral("--timeout-ms"),
                               QStringLiteral("25"),
                               QStringLiteral("list")});

    const ParseOptionsResult success = parseOptions(parser);
    QVERIFY(success.ok);
    QVERIFY(success.options.has_value());
    QCOMPARE(success.options->command, Command::List);
    QCOMPARE(success.options->jsonOutput, true);
    QCOMPARE(success.options->timeoutMs, 25);

    QCommandLineParser invalidCommandParser;
    configureParser(invalidCommandParser);
    invalidCommandParser.process(QStringList{QStringLiteral("window_probe"), QStringLiteral("nope")});
    const ParseOptionsResult invalidCommand = parseOptions(invalidCommandParser);
    QVERIFY(!invalidCommand.ok);
    QVERIFY(invalidCommand.errorMessage.contains(QStringLiteral("Unsupported command")));

    QCommandLineParser missingCommandParser;
    configureParser(missingCommandParser);
    missingCommandParser.process(QStringList{QStringLiteral("window_probe")});
    const ParseOptionsResult missingCommand = parseOptions(missingCommandParser);
    QVERIFY(!missingCommand.ok);
    QVERIFY(missingCommand.errorMessage.contains(QStringLiteral("exactly one command")));

    QCommandLineParser invalidTimeoutParser;
    configureParser(invalidTimeoutParser);
    invalidTimeoutParser.process(QStringList{QStringLiteral("window_probe"),
                                             QStringLiteral("--timeout-ms"),
                                             QStringLiteral("-1"),
                                             QStringLiteral("active")});
    const ParseOptionsResult invalidTimeout = parseOptions(invalidTimeoutParser);
    QVERIFY(!invalidTimeout.ok);
    QVERIFY(invalidTimeout.errorMessage.contains(QStringLiteral("timeout-ms")));
}

void WindowProbeRunnerTest::runnerPrintsListJsonAndExitsInOneShotMode()
{
    plasma_bridge::tests::FakeWindowProbeSource source;
    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::window_probe::WindowProbeOptions options;
    options.command = plasma_bridge::tools::window_probe::Command::List;
    options.jsonOutput = true;
    options.timeoutMs = 100;

    plasma_bridge::tools::window_probe::WindowProbeRunner runner(&source, options, &output, &error);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::window_probe::WindowProbeRunner::finished);

    QTimer::singleShot(0, &source, [&source]() {
        source.emitInitialSnapshotReady(plasma_bridge::tests::sampleWindowSnapshot());
    });

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 0);
    QCOMPARE(source.startCount(), 1);
    QVERIFY(outputText.contains(QStringLiteral("\"command\": \"list\"")));
    QVERIFY(outputText.contains(QStringLiteral("\"backend\": \"plasma-wayland\"")));
    QVERIFY(outputText.contains(QStringLiteral("\"activeWindowId\": \"window-editor\"")));
    QVERIFY(outputText.contains(QStringLiteral("\"title\": \"CHANGELOG.md - Kate\"")));
    QVERIFY(errorText.isEmpty());
}

void WindowProbeRunnerTest::runnerPrintsActiveJsonWithNullWindow()
{
    plasma_bridge::tests::FakeWindowProbeSource source;
    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::window_probe::WindowProbeOptions options;
    options.command = plasma_bridge::tools::window_probe::Command::Active;
    options.jsonOutput = true;
    options.timeoutMs = 100;

    plasma_bridge::tools::window_probe::WindowProbeRunner runner(&source, options, &output, &error);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::window_probe::WindowProbeRunner::finished);

    QTimer::singleShot(0, &source, [&source]() {
        source.emitInitialSnapshotReady(plasma_bridge::tests::sampleWindowSnapshotWithoutActiveWindow());
    });

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 0);
    QVERIFY(outputText.contains(QStringLiteral("\"command\": \"active\"")));
    QVERIFY(outputText.contains(QStringLiteral("\"window\": null")));
    QVERIFY(errorText.isEmpty());
}

void WindowProbeRunnerTest::runnerTimesOutBeforeInitialSnapshot()
{
    plasma_bridge::tests::FakeWindowProbeSource source;
    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::window_probe::WindowProbeOptions options;
    options.command = plasma_bridge::tools::window_probe::Command::List;
    options.timeoutMs = 10;

    plasma_bridge::tools::window_probe::WindowProbeRunner runner(&source, options, &output, &error);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::window_probe::WindowProbeRunner::finished);

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 1);
    QVERIFY(errorText.contains(QStringLiteral("Timed out waiting for Plasma Wayland window state.")));
}

void WindowProbeRunnerTest::runnerFailsWhenConnectionBreaksBeforeReady()
{
    plasma_bridge::tests::FakeWindowProbeSource source;
    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::window_probe::WindowProbeOptions options;
    options.command = plasma_bridge::tools::window_probe::Command::List;
    options.timeoutMs = 100;

    plasma_bridge::tools::window_probe::WindowProbeRunner runner(&source, options, &output, &error);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::window_probe::WindowProbeRunner::finished);

    QTimer::singleShot(0, &source, [&source]() {
        source.emitConnectionFailure(QStringLiteral("Synthetic backend failure."));
    });

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 1);
    QVERIFY(errorText.contains(QStringLiteral("Synthetic backend failure.")));
}

QTEST_GUILESS_MAIN(WindowProbeRunnerTest)

#include "test_window_probe_runner.moc"
