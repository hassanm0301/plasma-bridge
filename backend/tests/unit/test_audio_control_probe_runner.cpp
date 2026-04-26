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
    void parseCommandAndValueHelpers();
    void parseOptionsHandlesSuccessAndFailures();
    void runnerSubmitsAcceptedVolumeRequestImmediately();
    void runnerDispatchesDefaultAndMuteRequests();
    void runnerWaitsForSubmissionGateThenSubmits();
    void runnerSubmitsWhenTimeoutExpires();
};

void AudioControlProbeRunnerTest::parseCommandAndValueHelpers()
{
    using namespace plasma_bridge::tools::audio_control_probe;

    QCOMPARE(parseCommand(QStringLiteral("set")).value(), Command::SetVolume);
    QCOMPARE(parseCommand(QStringLiteral("increment")).value(), Command::IncrementVolume);
    QCOMPARE(parseCommand(QStringLiteral("decrement")).value(), Command::DecrementVolume);
    QCOMPARE(parseCommand(QStringLiteral("set-default-sink")).value(), Command::SetDefaultSink);
    QCOMPARE(parseCommand(QStringLiteral("set-default-source")).value(), Command::SetDefaultSource);
    QCOMPARE(parseCommand(QStringLiteral("set-sink-mute")).value(), Command::SetSinkMute);
    QCOMPARE(parseCommand(QStringLiteral("set-source-mute")).value(), Command::SetSourceMute);
    QVERIFY(!parseCommand(QStringLiteral("nope")).has_value());

    QCOMPARE(parseRequestedValue(QStringLiteral("0.25")).value(), 0.25);
    QVERIFY(std::isnan(*parseRequestedValue(QStringLiteral("nan"))));
    QVERIFY(std::isinf(*parseRequestedValue(QStringLiteral("inf"))));
    QVERIFY(!parseRequestedValue(QStringLiteral("not-a-number")).has_value());

    QCOMPARE(parseRequestedMuted(QStringLiteral("true")).value(), true);
    QCOMPARE(parseRequestedMuted(QStringLiteral("FALSE")).value(), false);
    QVERIFY(!parseRequestedMuted(QStringLiteral("maybe")).has_value());
}

void AudioControlProbeRunnerTest::parseOptionsHandlesSuccessAndFailures()
{
    using namespace plasma_bridge::tools::audio_control_probe;

    QCommandLineParser volumeParser;
    configureParser(volumeParser);
    volumeParser.process(QStringList{QStringLiteral("audio_control_probe"),
                                     QStringLiteral("--json"),
                                     QStringLiteral("--timeout-ms"),
                                     QStringLiteral("20"),
                                     QStringLiteral("set"),
                                     QStringLiteral("sink-1"),
                                     QStringLiteral("0.5")});

    const ParseOptionsResult volumeSuccess = parseOptions(volumeParser);
    QVERIFY(volumeSuccess.ok);
    QVERIFY(volumeSuccess.options.has_value());
    QCOMPARE(volumeSuccess.options->command, Command::SetVolume);
    QCOMPARE(volumeSuccess.options->deviceId, QStringLiteral("sink-1"));
    QVERIFY(volumeSuccess.options->requestedValue.has_value());
    QCOMPARE(*volumeSuccess.options->requestedValue, 0.5);
    QCOMPARE(volumeSuccess.options->timeoutMs, 20);
    QCOMPARE(volumeSuccess.options->jsonOutput, true);

    QCommandLineParser defaultParser;
    configureParser(defaultParser);
    defaultParser.process(QStringList{QStringLiteral("audio_control_probe"),
                                      QStringLiteral("set-default-source"),
                                      QStringLiteral("source-1")});

    const ParseOptionsResult defaultSuccess = parseOptions(defaultParser);
    QVERIFY(defaultSuccess.ok);
    QCOMPARE(defaultSuccess.options->command, Command::SetDefaultSource);
    QCOMPARE(defaultSuccess.options->deviceId, QStringLiteral("source-1"));
    QVERIFY(!defaultSuccess.options->requestedValue.has_value());
    QVERIFY(!defaultSuccess.options->requestedMuted.has_value());

    QCommandLineParser muteParser;
    configureParser(muteParser);
    muteParser.process(QStringList{QStringLiteral("audio_control_probe"),
                                   QStringLiteral("set-sink-mute"),
                                   QStringLiteral("sink-1"),
                                   QStringLiteral("false")});

    const ParseOptionsResult muteSuccess = parseOptions(muteParser);
    QVERIFY(muteSuccess.ok);
    QCOMPARE(muteSuccess.options->command, Command::SetSinkMute);
    QCOMPARE(muteSuccess.options->deviceId, QStringLiteral("sink-1"));
    QVERIFY(muteSuccess.options->requestedMuted.has_value());
    QCOMPARE(*muteSuccess.options->requestedMuted, false);

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

    QCommandLineParser invalidMuteParser;
    configureParser(invalidMuteParser);
    invalidMuteParser.process(QStringList{QStringLiteral("audio_control_probe"),
                                          QStringLiteral("set-source-mute"),
                                          QStringLiteral("source-1"),
                                          QStringLiteral("oops")});
    const ParseOptionsResult invalidMute = parseOptions(invalidMuteParser);
    QVERIFY(!invalidMute.ok);
    QVERIFY(invalidMute.errorMessage.contains(QStringLiteral("Muted")));

    QCommandLineParser missingArgsParser;
    configureParser(missingArgsParser);
    missingArgsParser.process(QStringList{QStringLiteral("audio_control_probe"),
                                          QStringLiteral("set-default-sink")});
    const ParseOptionsResult missingArgs = parseOptions(missingArgsParser);
    QVERIFY(!missingArgs.ok);
    QVERIFY(missingArgs.errorMessage.contains(QStringLiteral("expects")));
}

void AudioControlProbeRunnerTest::runnerSubmitsAcceptedVolumeRequestImmediately()
{
    plasma_bridge::tests::FakeAudioVolumeController volumeController;
    plasma_bridge::tests::FakeAudioDeviceController deviceController;
    plasma_bridge::tests::FakeSubmissionGate gate;
    gate.setShouldSubmitRequest(true);

    plasma_bridge::control::VolumeChangeResult accepted;
    accepted.status = plasma_bridge::control::VolumeChangeStatus::Accepted;
    accepted.sinkId = QStringLiteral("sink-1");
    accepted.requestedValue = 0.25;
    accepted.targetValue = 0.5;
    accepted.previousValue = 0.25;
    volumeController.setResult(plasma_bridge::tests::FakeAudioVolumeController::Operation::Set, accepted);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeOptions options;
    options.command = plasma_bridge::tools::audio_control_probe::Command::SetVolume;
    options.deviceId = QStringLiteral("sink-1");
    options.requestedValue = 0.5;
    options.jsonOutput = true;

    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner runner(&volumeController,
                                                                               &deviceController,
                                                                               &gate,
                                                                               options,
                                                                               &output,
                                                                               &error);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner::finished);

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 0);
    QCOMPARE(volumeController.lastOperation(), plasma_bridge::tests::FakeAudioVolumeController::Operation::Set);
    QCOMPARE(volumeController.lastSinkId(), QStringLiteral("sink-1"));
    QCOMPARE(volumeController.lastValue(), 0.5);
    QVERIFY(outputText.contains(QStringLiteral("\"status\": \"accepted\"")));
    QVERIFY(errorText.isEmpty());
}

void AudioControlProbeRunnerTest::runnerDispatchesDefaultAndMuteRequests()
{
    plasma_bridge::tests::FakeAudioVolumeController volumeController;
    plasma_bridge::tests::FakeAudioDeviceController deviceController;
    plasma_bridge::tests::FakeSubmissionGate gate;
    gate.setShouldSubmitRequest(true);

    plasma_bridge::control::DefaultDeviceChangeResult defaultResult;
    defaultResult.status = plasma_bridge::control::AudioDeviceChangeStatus::Accepted;
    defaultResult.deviceId = QStringLiteral("source-1");
    deviceController.setDefaultResult(plasma_bridge::tests::FakeAudioDeviceController::Operation::SetDefaultSource,
                                      defaultResult);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeOptions defaultOptions;
    defaultOptions.command = plasma_bridge::tools::audio_control_probe::Command::SetDefaultSource;
    defaultOptions.deviceId = QStringLiteral("source-1");

    QString defaultOutputText;
    QString defaultErrorText;
    QTextStream defaultOutput(&defaultOutputText);
    QTextStream defaultError(&defaultErrorText);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner defaultRunner(&volumeController,
                                                                                      &deviceController,
                                                                                      &gate,
                                                                                      defaultOptions,
                                                                                      &defaultOutput,
                                                                                      &defaultError);
    QSignalSpy defaultFinishedSpy(&defaultRunner, &plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner::finished);

    defaultRunner.start();

    QTRY_COMPARE(defaultFinishedSpy.count(), 1);
    QCOMPARE(defaultFinishedSpy.takeFirst().at(0).toInt(), 0);
    QCOMPARE(deviceController.lastOperation(), plasma_bridge::tests::FakeAudioDeviceController::Operation::SetDefaultSource);
    QCOMPARE(deviceController.lastDeviceId(), QStringLiteral("source-1"));
    QVERIFY(defaultOutputText.contains(QStringLiteral("Status: accepted")));
    QVERIFY(defaultErrorText.isEmpty());

    plasma_bridge::control::MuteChangeResult muteResult;
    muteResult.status = plasma_bridge::control::AudioDeviceChangeStatus::SourceNotWritable;
    muteResult.deviceId = QStringLiteral("source-1");
    muteResult.requestedMuted = true;
    muteResult.targetMuted = false;
    muteResult.previousMuted = false;
    deviceController.setMuteResult(plasma_bridge::tests::FakeAudioDeviceController::Operation::SetSourceMute, muteResult);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeOptions muteOptions;
    muteOptions.command = plasma_bridge::tools::audio_control_probe::Command::SetSourceMute;
    muteOptions.deviceId = QStringLiteral("source-1");
    muteOptions.requestedMuted = true;
    muteOptions.jsonOutput = true;

    QString muteOutputText;
    QString muteErrorText;
    QTextStream muteOutput(&muteOutputText);
    QTextStream muteError(&muteErrorText);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner muteRunner(&volumeController,
                                                                                   &deviceController,
                                                                                   &gate,
                                                                                   muteOptions,
                                                                                   &muteOutput,
                                                                                   &muteError);
    QSignalSpy muteFinishedSpy(&muteRunner, &plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner::finished);

    muteRunner.start();

    QTRY_COMPARE(muteFinishedSpy.count(), 1);
    QCOMPARE(muteFinishedSpy.takeFirst().at(0).toInt(), 1);
    QCOMPARE(deviceController.lastOperation(), plasma_bridge::tests::FakeAudioDeviceController::Operation::SetSourceMute);
    QCOMPARE(deviceController.lastDeviceId(), QStringLiteral("source-1"));
    QVERIFY(deviceController.lastMuted().has_value());
    QCOMPARE(*deviceController.lastMuted(), true);
    QVERIFY(muteOutputText.contains(QStringLiteral("\"status\": \"source_not_writable\"")));
    QVERIFY(muteErrorText.isEmpty());
}

void AudioControlProbeRunnerTest::runnerWaitsForSubmissionGateThenSubmits()
{
    plasma_bridge::tests::FakeAudioVolumeController volumeController;
    plasma_bridge::tests::FakeAudioDeviceController deviceController;
    plasma_bridge::tests::FakeSubmissionGate gate;

    plasma_bridge::control::DefaultDeviceChangeResult result;
    result.status = plasma_bridge::control::AudioDeviceChangeStatus::SinkNotFound;
    result.deviceId = QStringLiteral("missing-sink");
    deviceController.setDefaultResult(plasma_bridge::tests::FakeAudioDeviceController::Operation::SetDefaultSink, result);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeOptions options;
    options.command = plasma_bridge::tools::audio_control_probe::Command::SetDefaultSink;
    options.deviceId = QStringLiteral("missing-sink");
    options.timeoutMs = 100;

    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner runner(&volumeController,
                                                                               &deviceController,
                                                                               &gate,
                                                                               options,
                                                                               &output,
                                                                               &error);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner::finished);

    QTimer::singleShot(10, &gate, [&gate]() {
        gate.setShouldSubmitRequest(true);
    });

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 1);
    QCOMPARE(deviceController.lastOperation(), plasma_bridge::tests::FakeAudioDeviceController::Operation::SetDefaultSink);
    QVERIFY(outputText.contains(QStringLiteral("Status: sink_not_found")));
}

void AudioControlProbeRunnerTest::runnerSubmitsWhenTimeoutExpires()
{
    plasma_bridge::tests::FakeAudioVolumeController volumeController;
    plasma_bridge::tests::FakeAudioDeviceController deviceController;
    plasma_bridge::tests::FakeSubmissionGate gate;

    plasma_bridge::control::MuteChangeResult result;
    result.status = plasma_bridge::control::AudioDeviceChangeStatus::NotReady;
    result.deviceId = QStringLiteral("sink-1");
    result.requestedMuted = false;
    deviceController.setMuteResult(plasma_bridge::tests::FakeAudioDeviceController::Operation::SetSinkMute, result);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeOptions options;
    options.command = plasma_bridge::tools::audio_control_probe::Command::SetSinkMute;
    options.deviceId = QStringLiteral("sink-1");
    options.requestedMuted = false;
    options.timeoutMs = 10;

    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner runner(&volumeController,
                                                                               &deviceController,
                                                                               &gate,
                                                                               options,
                                                                               &output,
                                                                               &error);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::audio_control_probe::AudioControlProbeRunner::finished);

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 1);
    QCOMPARE(deviceController.lastOperation(), plasma_bridge::tests::FakeAudioDeviceController::Operation::SetSinkMute);
    QVERIFY(outputText.contains(QStringLiteral("Status: not_ready")));
}

QTEST_GUILESS_MAIN(AudioControlProbeRunnerTest)

#include "test_audio_control_probe_runner.moc"
