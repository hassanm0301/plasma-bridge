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
    void windowProbeHelpAndHermeticPaths();

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
    QVERIFY(timeout.standardError.contains(QStringLiteral("Timed out waiting for PulseAudioQt audio state.")));

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
    QVERIFY(help.standardOutput.contains(QStringLiteral("set-default-sink")));
    QVERIFY(help.standardOutput.contains(QStringLiteral("set-source-mute")));

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

    const ProcessResult invalidMuted = runProcess(audioControlProbeBin,
                                                  {QStringLiteral("set-sink-mute"),
                                                   QStringLiteral("sink-1"),
                                                   QStringLiteral("maybe")});
    QCOMPARE(invalidMuted.exitCode, 1);
    QVERIFY(invalidMuted.standardError.contains(QStringLiteral("Muted must be true or false")));

    const ProcessResult missingArgs = runProcess(audioControlProbeBin,
                                                 {QStringLiteral("set-default-source")});
    QCOMPARE(missingArgs.exitCode, 1);
    QVERIFY(missingArgs.standardError.contains(QStringLiteral("expects")));
}

void CliBinariesFeatureTest::windowProbeHelpAndHermeticPaths()
{
    const QString windowProbeBin = QString::fromUtf8(TEST_WINDOW_PROBE_BIN);
    const QString fakeWindowProbeCliBin = QString::fromUtf8(TEST_FAKE_WINDOW_PROBE_CLI_BIN);

    const ProcessResult help = runProcess(windowProbeBin, {QStringLiteral("--help")});
    QCOMPARE(help.exitCode, 0);
    QVERIFY(help.standardOutput.contains(QStringLiteral("--timeout-ms")));
    QVERIFY(help.standardOutput.contains(QStringLiteral("list")));
    QVERIFY(help.standardOutput.contains(QStringLiteral("active")));

    const ProcessResult timeout = runProcess(fakeWindowProbeCliBin,
                                             {QStringLiteral("--scenario"),
                                              QStringLiteral("timeout"),
                                              QStringLiteral("--timeout-ms"),
                                              QStringLiteral("20"),
                                              QStringLiteral("list")});
    QCOMPARE(timeout.exitCode, 1);
    QVERIFY(timeout.standardError.contains(QStringLiteral("Timed out waiting for Plasma Wayland window state.")));

    const ProcessResult failure = runProcess(fakeWindowProbeCliBin,
                                             {QStringLiteral("--scenario"), QStringLiteral("failure"), QStringLiteral("active")});
    QCOMPARE(failure.exitCode, 1);
    QVERIFY(failure.standardError.contains(QStringLiteral("Synthetic backend failure.")));

    const ProcessResult list = runProcess(fakeWindowProbeCliBin,
                                          {QStringLiteral("--scenario"),
                                           QStringLiteral("list"),
                                           QStringLiteral("--json"),
                                           QStringLiteral("list")});
    QCOMPARE(list.exitCode, 0);
    QVERIFY(list.standardOutput.contains(QStringLiteral("\"activeWindowId\": \"window-editor\"")));

    const ProcessResult noActive = runProcess(fakeWindowProbeCliBin,
                                              {QStringLiteral("--scenario"),
                                               QStringLiteral("no-active"),
                                               QStringLiteral("--json"),
                                               QStringLiteral("active")});
    QCOMPARE(noActive.exitCode, 0);
    QVERIFY(noActive.standardOutput.contains(QStringLiteral("\"window\": null")));
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
