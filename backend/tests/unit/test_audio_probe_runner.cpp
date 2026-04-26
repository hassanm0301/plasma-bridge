#include "tests/support/test_support.h"
#include "tools/probes/audio_probe/audio_probe_runner.h"

#include <QCommandLineParser>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>

class AudioProbeRunnerTest : public QObject
{
    Q_OBJECT

private slots:
    void parserOptionsReflectFlags();
    void runnerPrintsIndentedJsonAndExitsInOneShotMode();
    void runnerPrintsHumanEventsInWatchMode();
    void runnerTimesOutBeforeInitialState();
    void runnerFailsWhenConnectionBreaksBeforeReady();
};

void AudioProbeRunnerTest::parserOptionsReflectFlags()
{
    QCommandLineParser parser;
    plasma_bridge::tools::audio_probe::configureParser(parser);
    parser.process(QStringList{QStringLiteral("audio_probe"), QStringLiteral("--watch"), QStringLiteral("--json")});

    const plasma_bridge::tools::audio_probe::AudioProbeOptions options =
        plasma_bridge::tools::audio_probe::optionsFromParser(parser);

    QCOMPARE(options.watchMode, true);
    QCOMPARE(options.jsonOutput, true);
}

void AudioProbeRunnerTest::runnerPrintsIndentedJsonAndExitsInOneShotMode()
{
    plasma_bridge::tests::FakeAudioProbeSource source;
    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::audio_probe::AudioProbeOptions options;
    options.jsonOutput = true;

    plasma_bridge::tools::audio_probe::AudioProbeRunner runner(&source, options, &output, &error, 100);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::audio_probe::AudioProbeRunner::finished);

    QTimer::singleShot(0, &source, [&source]() {
        source.emitInitialStateReady(plasma_bridge::tests::sampleAudioState());
    });

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 0);
    QCOMPARE(source.startCount(), 1);
    QVERIFY(outputText.contains(QStringLiteral("\n")));
    QVERIFY(outputText.contains(QStringLiteral("\"event\": \"audioState\"")));
    QVERIFY(outputText.contains(QStringLiteral("\"reason\": \"initial\"")));
    QVERIFY(errorText.isEmpty());
}

void AudioProbeRunnerTest::runnerPrintsHumanEventsInWatchMode()
{
    plasma_bridge::tests::FakeAudioProbeSource source;
    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::audio_probe::AudioProbeOptions options;
    options.watchMode = true;

    plasma_bridge::tools::audio_probe::AudioProbeRunner runner(&source, options, &output, &error, 100);

    QTimer::singleShot(0, &source, [&source]() {
        source.emitInitialStateReady(plasma_bridge::tests::sampleAudioState());
        source.emitAudioStateChanged(QStringLiteral("sink-updated"),
                                     QStringLiteral("bluez_output.headset.1"),
                                     QString(),
                                     plasma_bridge::tests::alternateAudioState());
    });

    runner.start();

    QTRY_VERIFY(outputText.contains(QStringLiteral("Reason: initial")));
    QTRY_VERIFY(outputText.contains(QStringLiteral("Reason: sink-updated")));
    QVERIFY(outputText.contains(QStringLiteral("Sink: bluez_output.headset.1")));
    QVERIFY(outputText.contains(QStringLiteral("Selected Source: bluez_input.headset.1")));
    QVERIFY(errorText.isEmpty());
}

void AudioProbeRunnerTest::runnerTimesOutBeforeInitialState()
{
    plasma_bridge::tests::FakeAudioProbeSource source;
    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::audio_probe::AudioProbeRunner runner(&source, {}, &output, &error, 10);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::audio_probe::AudioProbeRunner::finished);

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 1);
    QVERIFY(errorText.contains(QStringLiteral("Timed out waiting for PulseAudioQt audio state.")));
}

void AudioProbeRunnerTest::runnerFailsWhenConnectionBreaksBeforeReady()
{
    plasma_bridge::tests::FakeAudioProbeSource source;
    QString outputText;
    QString errorText;
    QTextStream output(&outputText);
    QTextStream error(&errorText);

    plasma_bridge::tools::audio_probe::AudioProbeRunner runner(&source, {}, &output, &error, 100);
    QSignalSpy finishedSpy(&runner, &plasma_bridge::tools::audio_probe::AudioProbeRunner::finished);

    QTimer::singleShot(0, &source, [&source]() {
        source.emitConnectionFailure(QStringLiteral("Synthetic backend failure."));
    });

    runner.start();

    QTRY_COMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.takeFirst().at(0).toInt(), 1);
    QVERIFY(errorText.contains(QStringLiteral("Synthetic backend failure.")));
}

QTEST_GUILESS_MAIN(AudioProbeRunnerTest)

#include "test_audio_probe_runner.moc"
