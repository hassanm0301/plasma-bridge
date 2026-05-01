#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
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
    void mediaProbeHelpAndHermeticPaths();
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
    QVERIFY(help.standardOutput.contains(QStringLiteral("--allow-origin")));
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

    const ProcessResult invalidAllowedOrigin =
        runProcess(plasmaBridgeBin, {QStringLiteral("--allow-origin"), QStringLiteral("http://example.test/dashboard")});
    QCOMPARE(invalidAllowedOrigin.exitCode, 1);
    QVERIFY(invalidAllowedOrigin.standardError.contains(QStringLiteral("Invalid allowed origin")));
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

void CliBinariesFeatureTest::mediaProbeHelpAndHermeticPaths()
{
    const QString mediaProbeBin = QString::fromUtf8(TEST_MEDIA_PROBE_BIN);
    const QString fakeMediaProbeCliBin = QString::fromUtf8(TEST_FAKE_MEDIA_PROBE_CLI_BIN);

    const ProcessResult help = runProcess(mediaProbeBin, {QStringLiteral("--help")});
    QCOMPARE(help.exitCode, 0);
    QVERIFY(help.standardOutput.contains(QStringLiteral("--watch")));
    QVERIFY(help.standardOutput.contains(QStringLiteral("--json")));
    QVERIFY(help.standardOutput.contains(QStringLiteral("--timeout-ms")));
    QVERIFY(help.standardOutput.contains(QStringLiteral("play-pause")));

    const ProcessResult invalidCommand = runProcess(mediaProbeBin, {QStringLiteral("seek")});
    QCOMPARE(invalidCommand.exitCode, 1);
    QVERIFY(invalidCommand.standardError.contains(QStringLiteral("Unsupported command")));

    const ProcessResult invalidWatch = runProcess(mediaProbeBin, {QStringLiteral("--watch"), QStringLiteral("play")});
    QCOMPARE(invalidWatch.exitCode, 1);
    QVERIFY(invalidWatch.standardError.contains(QStringLiteral("--watch")));

    const ProcessResult timeout = runProcess(fakeMediaProbeCliBin,
                                             {QStringLiteral("--scenario"),
                                              QStringLiteral("timeout"),
                                              QStringLiteral("--timeout-ms"),
                                              QStringLiteral("20")});
    QCOMPARE(timeout.exitCode, 1);
    QVERIFY(timeout.standardError.contains(QStringLiteral("Timed out waiting for MPRIS media state.")));

    const ProcessResult failure = runProcess(fakeMediaProbeCliBin, {QStringLiteral("--scenario"), QStringLiteral("failure")});
    QCOMPARE(failure.exitCode, 1);
    QVERIFY(failure.standardError.contains(QStringLiteral("Synthetic media failure.")));

    const ProcessResult current = runProcess(fakeMediaProbeCliBin,
                                             {QStringLiteral("--scenario"), QStringLiteral("current"), QStringLiteral("--json")});
    QCOMPARE(current.exitCode, 0);
    QVERIFY(current.standardOutput.contains(QStringLiteral("\"event\": \"mediaState\"")));
    QVERIFY(current.standardOutput.contains(QStringLiteral("\"playerId\": \"org.mpris.MediaPlayer2.spotify\"")));

    const ProcessResult noPlayer = runProcess(fakeMediaProbeCliBin,
                                              {QStringLiteral("--scenario"),
                                               QStringLiteral("no-player"),
                                               QStringLiteral("--json"),
                                               QStringLiteral("current")});
    QCOMPARE(noPlayer.exitCode, 0);
    QVERIFY(noPlayer.standardOutput.contains(QStringLiteral("\"player\": null")));

    const ProcessResult playPauseFailure = runProcess(fakeMediaProbeCliBin,
                                                      {QStringLiteral("--scenario"),
                                                       QStringLiteral("control-failure"),
                                                       QStringLiteral("--json"),
                                                       QStringLiteral("play-pause")});
    QCOMPARE(playPauseFailure.exitCode, 1);
    QVERIFY(playPauseFailure.standardOutput.contains(QStringLiteral("\"command\": \"play-pause\"")));
    QVERIFY(playPauseFailure.standardOutput.contains(QStringLiteral("\"status\": \"action_not_supported\"")));
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
    QVERIFY(help.standardOutput.contains(QStringLiteral("activate")));
    QVERIFY(help.standardOutput.contains(QStringLiteral("setup")));
    QVERIFY(help.standardOutput.contains(QStringLiteral("status")));
    QVERIFY(help.standardOutput.contains(QStringLiteral("teardown")));

    const ProcessResult status = runProcess(windowProbeBin, {QStringLiteral("--json"), QStringLiteral("status")});
    QCOMPARE(status.exitCode, 0);
    QVERIFY(status.standardOutput.contains(QStringLiteral("\"command\": \"status\"")));
    QVERIFY(status.standardOutput.contains(QStringLiteral("\"scriptInstalled\": false")));

    const ProcessResult missingSetup = runProcess(windowProbeBin, {QStringLiteral("list")});
    QCOMPARE(missingSetup.exitCode, 1);
    QVERIFY(missingSetup.standardError.contains(QStringLiteral("window_probe backend is not configured")));

    const ProcessResult timeout = runProcess(fakeWindowProbeCliBin,
                                             {QStringLiteral("--scenario"),
                                              QStringLiteral("timeout"),
                                              QStringLiteral("--timeout-ms"),
                                              QStringLiteral("20"),
                                              QStringLiteral("list")});
    QCOMPARE(timeout.exitCode, 1);
    QVERIFY(timeout.standardError.contains(QStringLiteral("Timed out waiting for cached KWin script window state.")));

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
    QVERIFY(list.standardOutput.contains(QStringLiteral("\"backend\": \"kwin-script-helper\"")));

    const ProcessResult noActive = runProcess(fakeWindowProbeCliBin,
                                              {QStringLiteral("--scenario"),
                                               QStringLiteral("no-active"),
                                               QStringLiteral("--json"),
                                               QStringLiteral("active")});
    QCOMPARE(noActive.exitCode, 0);
    QVERIFY(noActive.standardOutput.contains(QStringLiteral("\"window\": null")));

    const ProcessResult activate = runProcess(fakeWindowProbeCliBin,
                                              {QStringLiteral("--scenario"),
                                               QStringLiteral("list"),
                                               QStringLiteral("--json"),
                                               QStringLiteral("activate"),
                                               QStringLiteral("window-terminal")});
    QCOMPARE(activate.exitCode, 0);
    QVERIFY(activate.standardOutput.contains(QStringLiteral("\"command\": \"activate\"")));
    QVERIFY(activate.standardOutput.contains(QStringLiteral("\"status\": \"accepted\"")));
    QVERIFY(activate.standardOutput.contains(QStringLiteral("\"windowId\": \"window-terminal\"")));

    const ProcessResult activationFailure = runProcess(fakeWindowProbeCliBin,
                                                       {QStringLiteral("--scenario"),
                                                        QStringLiteral("activation-failure"),
                                                        QStringLiteral("--json"),
                                                        QStringLiteral("activate"),
                                                        QStringLiteral("window-terminal")});
    QCOMPARE(activationFailure.exitCode, 1);
    QVERIFY(activationFailure.standardOutput.contains(QStringLiteral("\"status\": \"window_not_activatable\"")));
}

CliBinariesFeatureTest::ProcessResult CliBinariesFeatureTest::runProcess(const QString &program,
                                                                         const QStringList &arguments,
                                                                         const int timeoutMs)
{
    ProcessResult result;
    QProcess process;
    QTemporaryDir tempRoot;
    if (!tempRoot.isValid()) {
        result.standardError = QStringLiteral("Failed to create temporary test root.");
        return result;
    }
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("PLASMA_BRIDGE_WINDOW_PROBE_DATA_ROOT"), tempRoot.path());
    environment.insert(QStringLiteral("PLASMA_BRIDGE_WINDOW_PROBE_CONFIG_ROOT"), tempRoot.path());
    process.setProcessEnvironment(environment);
    process.start(program, arguments);

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
