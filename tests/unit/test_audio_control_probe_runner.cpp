#include "tests/support/test_support.h"
#include "tools/probes/audio_control_probe/audio_control_probe_runner.h"

#include <QCommandLineParser>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>

#include <cmath>
#include <limits>

class AudioControlProbeRunnerTest : public QObject
{
    Q_OBJECT

private slots:
    void parseCommandAndRequestedValueHelpers();
    void parseOptionsHandlesSuccessAndFailures();
    void runnerSubmitsAcceptedRequestImmediately();
    void runnerWaitsForSubmissionGateThenSubmits();
    void runnerSubmitsWhenTimeoutExpires();
};

void AudioControlProbeRunnerTest::parseCommandAndRequestedValueHelpers()
{
    using namespace plasma_bridge::tools::audio_control_probe;

    QCOMPARE(parseCommand(QStringLiteral("set")).value(), Command::Set);
    QCOMPARE(parseCommand(QStringLiteral("increment")).value(), Command::Increment);
    QCOMPARE(parseCommand(QStringLiteral("decrement")).value(), Command::Decrement);
    QVERIFY(!parseCommand(QStringLiteral("nope")).has_value());

    QCOMPARE(parseRequestedValue(QStringLiteral("0.25")).value(), 0.25);
    QVERIFY(std::isnan(*parseRequestedValue(QStringLiteral("nan"))));
    QVERIFY(std::isinf(*parseRequestedValue(QStringLiteral("inf"))));
    QVERIFY(!parseRequestedValue(QStringLiteral("not-a-number")).has_value());
}

void AudioControlProbeRunnerTest::parseOptionsHandlesSuccessAndFailures()
{
    using namespace plasma_bridge::tools::audio_control_probe;

    QCommandLineParser parser;
    configureParser(parser);
    parser.process(QStringList{QStringLiteral("audio_control_probe"),
                               QStringLiteral("--json"),
                               QStringLiteral("--timeout-ms"),
                               QStringLiteral("20"),
                               QStringLiteral("set"),
                               QStringLiteral("sink-1"),
                               QStringLiteral("0.5")});

    const ParseOptionsResult success = parseOptions(parser);
    QVERIFY(success.ok);
    QVERIFY(success.options.has_value());
    QCOMPARE(success.options->command, Command::Set);
    QCOMPARE(success.options->sinkId, QStringLiteral("sink-1"));
    QCOMPARE(success.options->requestedValue, 0.5);
    QCOMPARE(success.options->timeoutMs, 20);
    QCOMPARE(success.options->jsonOutput, true);

    QCommandLineParser invalidTimeoutParser;
    configureParser(invalidTimeoutParser);
    invalidTimeoutParser.process(QStringList{QStringLiteral("audio_control_probe"),
                                             QStringLiteral("--timeout-ms"),
                                             QStringLiteral("-1"),
                                             QStringLiteral("set"),
                                             QStringLiteral("sink-1"),
                                             QStringLiteral("0.5")});
    const ParseOptionsResult invalidTimeout = parseOptions(invalidTimeoutParser);
    QVERIFY(!invalidTimeout.ok);
    QVERIFY(invalidTimeout.errorMessage.contains(QStringLiteral("timeout-ms")));

    QCommandLineParser invalidCommandParser;
    configureParser(invalidCommandParser);
    invalidCommandParser.process(QStringList{QStringLiteral("audio_control_probe"),
                                             QStringLiteral("nope"),
                                             QStringLiteral("sink-1"),
                                             QStringLiteral("0.5")});
    const ParseOptionsResult invalidCommand = parseOptions(invalidCommandParser);
    QVERIFY(!invalidCommand.ok);
    QVERIFY(invalidCommand.errorMessage.contains(QStringLiteral("Unsupported command")));
}

void AudioControlProbeRunnerTest::runnerSubmitsAcceptedRequestImmediately()
{
    plasma_bridge::tests::FakeAudioVolumeController controller;
    plasma_bridge::tests::FakeSubmissionGate gate;
    gate.setShouldSubmitRequest(true);

    plasma_bridge::control::VolumeChangeResult accepted;
    accepted.status = plasma_bridge::control::VolumeChangeStatus::Accepted;
    accepted.sinkId = QStringLiteral("sink-1");
    accepted.requestedValue = 0.25;
    accepted.targetValue = 0.5;
    accepted.previousValue = 0.25;
    controller.setResult(plasma_bridge::tests::FakeAudioVolumeController::Operation::Set, accepted);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeOptions options;
    options.command = plasma_bridge::tools::audio_control_probe::Command::Set;
    options.sinkId = QStringLiteral("sink-1");
    options.requestedValue = 0.5;
    options.jsonOutput = true;

    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner runner(&controller, &gate, options, &output, &error);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner::finished);

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 0);
    QCOMPARE(controller.lastOperation(), plasma_bridge::tests::FakeAudioVolumeController::Operation::Set);
    QCOMPARE(controller.lastSinkId(), QStringLiteral("sink-1"));
    QCOMPARE(controller.lastValue(), 0.5);
    QVERIFY(outputText.contains(QStringLiteral("\"status\": \"accepted\"")));
    QVERIFY(errorText.isEmpty());
}

void AudioControlProbeRunnerTest::runnerWaitsForSubmissionGateThenSubmits()
{
    plasma_bridge::tests::FakeAudioVolumeController controller;
    plasma_bridge::tests::FakeSubmissionGate gate;

    plasma_bridge::control::VolumeChangeResult result;
    result.status = plasma_bridge::control::VolumeChangeStatus::SinkNotFound;
    result.sinkId = QStringLiteral("missing-sink");
    result.requestedValue = 0.1;
    controller.setResult(plasma_bridge::tests::FakeAudioVolumeController::Operation::Increment, result);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeOptions options;
    options.command = plasma_bridge::tools::audio_control_probe::Command::Increment;
    options.sinkId = QStringLiteral("missing-sink");
    options.requestedValue = 0.1;
    options.timeoutMs = 100;

    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner runner(&controller, &gate, options, &output, &error);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner::finished);

    QTimer::singleShot(10, &gate, [&gate]() {
        gate.setShouldSubmitRequest(true);
    });

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 1);
    QCOMPARE(controller.lastOperation(), plasma_bridge::tests::FakeAudioVolumeController::Operation::Increment);
    QVERIFY(outputText.contains(QStringLiteral("Status: sink_not_found")));
}

void AudioControlProbeRunnerTest::runnerSubmitsWhenTimeoutExpires()
{
    plasma_bridge::tests::FakeAudioVolumeController controller;
    plasma_bridge::tests::FakeSubmissionGate gate;

    plasma_bridge::control::VolumeChangeResult result;
    result.status = plasma_bridge::control::VolumeChangeStatus::NotReady;
    result.sinkId = QStringLiteral("sink-1");
    result.requestedValue = 0.1;
    controller.setResult(plasma_bridge::tests::FakeAudioVolumeController::Operation::Decrement, result);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeOptions options;
    options.command = plasma_bridge::tools::audio_control_probe::Command::Decrement;
    options.sinkId = QStringLiteral("sink-1");
    options.requestedValue = 0.1;
    options.timeoutMs = 10;

    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner runner(&controller, &gate, options, &output, &error);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner::finished);

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 1);
    QCOMPARE(controller.lastOperation(), plasma_bridge::tests::FakeAudioVolumeController::Operation::Decrement);
    QVERIFY(outputText.contains(QStringLiteral("Status: not_ready")));
}

QTEST_GUILESS_MAIN(AudioControlProbeRunnerTest)

#include "test_audio_control_probe_runner.moc"
