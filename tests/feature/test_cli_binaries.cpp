#include <QProcess>
#include <QtTest>

class CliBinariesFeatureTest : public QObject
{
    Q_OBJECT

private:
    struct ProcessResult {
        int exitCode = -1;
        QString standardOutput;
        QString standardError;
    };

private slots:
    void plasmaBridgeHelpAndValidationErrors();
    void audioProbeHelpAndHermeticFailurePaths();
    void audioControlProbeHelpAndValidationErrors();

private:
    static ProcessResult runProcess(const QString &program, const QStringList &arguments, int timeoutMs = 5000);
};

void CliBinariesFeatureTest::plasmaBridgeHelpAndValidationErrors()
{
    const QString plasmaBridgeBin = QString::fromUtf8(TEST_PLASMA_BRIDGE_BIN);

    const ProcessResult help = runProcess(plasmaBridgeBin, {QStringLiteral("--help")});
    QCOMPARE(help.exitCode, 0);
    QVERIFY(help.standardOutput.contains(QStringLiteral("--host")));
    QVERIFY(help.standardOutput.contains(QStringLiteral("--ws-port")));

    const ProcessResult invalidHost = runProcess(plasmaBridgeBin, {QStringLiteral("--host"), QStringLiteral("not-an-address")});
    QCOMPARE(invalidHost.exitCode, 1);
    QVERIFY(invalidHost.standardError.contains(QStringLiteral("Invalid host address")));

    const ProcessResult invalidPort = runProcess(plasmaBridgeBin, {QStringLiteral("--port"), QStringLiteral("abc")});
    QCOMPARE(invalidPort.exitCode, 1);
    QVERIFY(invalidPort.standardError.contains(QStringLiteral("Invalid port")));

    const ProcessResult invalidWsPort = runProcess(plasmaBridgeBin, {QStringLiteral("--ws-port"), QStringLiteral("abc")});
    QCOMPARE(invalidWsPort.exitCode, 1);
    QVERIFY(invalidWsPort.standardError.contains(QStringLiteral("Invalid WebSocket port")));
}

void CliBinariesFeatureTest::audioProbeHelpAndHermeticFailurePaths()
{
    const QString audioProbeBin = QString::fromUtf8(TEST_AUDIO_PROBE_BIN);
    const QString fakeAudioProbeCliBin = QString::fromUtf8(TEST_FAKE_AUDIO_PROBE_CLI_BIN);

    const ProcessResult help = runProcess(audioProbeBin, {QStringLiteral("--help")});
    QCOMPARE(help.exitCode, 0);
    QVERIFY(help.standardOutput.contains(QStringLiteral("--watch")));
    QVERIFY(help.standardOutput.contains(QStringLiteral("--json")));

    const ProcessResult timeout = runProcess(fakeAudioProbeCliBin, {QStringLiteral("--scenario"), QStringLiteral("timeout")});
    QCOMPARE(timeout.exitCode, 1);
    QVERIFY(timeout.standardError.contains(QStringLiteral("Timed out waiting for PulseAudioQt sink state.")));

    const ProcessResult failure = runProcess(fakeAudioProbeCliBin, {QStringLiteral("--scenario"), QStringLiteral("failure")});
    QCOMPARE(failure.exitCode, 1);
    QVERIFY(failure.standardError.contains(QStringLiteral("Synthetic connection failure.")));
}

void CliBinariesFeatureTest::audioControlProbeHelpAndValidationErrors()
{
    const QString audioControlProbeBin = QString::fromUtf8(TEST_AUDIO_CONTROL_PROBE_BIN);

    const ProcessResult help = runProcess(audioControlProbeBin, {QStringLiteral("--help")});
    QCOMPARE(help.exitCode, 0);
    QVERIFY(help.standardOutput.contains(QStringLiteral("--timeout-ms")));
    QVERIFY(help.standardOutput.contains(QStringLiteral("set, increment, decrement")));

    const ProcessResult invalidCommand = runProcess(audioControlProbeBin,
                                                    {QStringLiteral("nope"), QStringLiteral("sink-1"), QStringLiteral("0.5")});
    QCOMPARE(invalidCommand.exitCode, 1);
    QVERIFY(invalidCommand.standardError.contains(QStringLiteral("Unsupported command")));

    const ProcessResult invalidValue = runProcess(audioControlProbeBin,
                                                  {QStringLiteral("set"), QStringLiteral("sink-1"), QStringLiteral("oops")});
    QCOMPARE(invalidValue.exitCode, 1);
    QVERIFY(invalidValue.standardError.contains(QStringLiteral("Value must be numeric")));

    const ProcessResult invalidTimeout = runProcess(audioControlProbeBin,
                                                    {QStringLiteral("--timeout-ms"),
                                                     QStringLiteral("-1"),
                                                     QStringLiteral("set"),
                                                     QStringLiteral("sink-1"),
                                                     QStringLiteral("0.5")});
    QCOMPARE(invalidTimeout.exitCode, 1);
    QVERIFY(invalidTimeout.standardError.contains(QStringLiteral("timeout-ms")));
}

CliBinariesFeatureTest::ProcessResult CliBinariesFeatureTest::runProcess(const QString &program,
                                                                         const QStringList &arguments,
                                                                         const int timeoutMs)
{
    QProcess process;
    process.start(program, arguments);

    ProcessResult result;
    if (!process.waitForStarted()) {
        result.standardError = process.errorString();
        return result;
    }

    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished();
        result.standardError = process.errorString();
        return result;
    }

    result.exitCode = process.exitCode();
    result.standardOutput = QString::fromUtf8(process.readAllStandardOutput());
    result.standardError = QString::fromUtf8(process.readAllStandardError());
    return result;
}

QTEST_GUILESS_MAIN(CliBinariesFeatureTest)

#include "test_cli_binaries.moc"
