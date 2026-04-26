#pragma once

#include "plasma_bridge_build_config.h"
#include "common/window_state.h"

#include <QObject>
#include <QString>

#include <optional>
#include <memory>

class QCommandLineParser;
class QTextStream;
class QTimer;

namespace plasma_bridge::tools::window_probe
{

enum class Command {
    List,
    Active,
    Setup,
    Status,
    Teardown,
};

struct WindowProbeOptions {
    Command command = Command::List;
    int timeoutMs = PLASMA_BRIDGE_DEFAULT_PROBE_STARTUP_TIMEOUT_MS;
    bool jsonOutput = false;
};

struct ParseOptionsResult {
    bool ok = false;
    std::optional<WindowProbeOptions> options;
    QString errorMessage;
};

struct WindowProbeBackendStatus {
    QString backendName;
    bool scriptInstalled = false;
    bool scriptEnabled = false;
    bool helperServiceInstalled = false;
    bool helperServiceRegistered = false;
    bool snapshotCached = false;
    bool snapshotReady = false;
};

struct WindowProbeCommandResult {
    bool ok = false;
    QString message;
    WindowProbeBackendStatus status;
};

class WindowProbeSource : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~WindowProbeSource() override = default;

    virtual void start() = 0;
    virtual const plasma_bridge::WindowSnapshot &currentSnapshot() const = 0;
    virtual bool hasInitialSnapshot() const = 0;
    virtual QString backendName() const = 0;

signals:
    void initialSnapshotReady();
    void connectionFailed(const QString &message);
};

class WindowProbeBackendController : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~WindowProbeBackendController() override = default;

    virtual WindowProbeCommandResult setup() = 0;
    virtual WindowProbeCommandResult status() = 0;
    virtual WindowProbeCommandResult teardown() = 0;
};

class KWinScriptWindowProbeBackendController final : public WindowProbeBackendController
{
    Q_OBJECT

public:
    explicit KWinScriptWindowProbeBackendController(QObject *parent = nullptr);
    ~KWinScriptWindowProbeBackendController() override;

    WindowProbeCommandResult setup() override;
    WindowProbeCommandResult status() override;
    WindowProbeCommandResult teardown() override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

class KWinScriptWindowProbeSource final : public WindowProbeSource
{
    Q_OBJECT

public:
    explicit KWinScriptWindowProbeSource(QObject *parent = nullptr);
    ~KWinScriptWindowProbeSource() override;

    void start() override;
    const plasma_bridge::WindowSnapshot &currentSnapshot() const override;
    bool hasInitialSnapshot() const override;
    QString backendName() const override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

class WindowProbeRunner final : public QObject
{
    Q_OBJECT

public:
    explicit WindowProbeRunner(WindowProbeSource *source,
                               WindowProbeBackendController *backendController,
                               const WindowProbeOptions &options,
                               QTextStream *output,
                               QTextStream *error,
                               QObject *parent = nullptr);

    void start();

signals:
    void finished(int exitCode);

private:
    void executeBackendCommand();
    void finish(int exitCode);
    void publishInitialSnapshot();
    void printSnapshot() const;
    void printBackendCommandResult(const WindowProbeCommandResult &result) const;

    WindowProbeSource *m_source = nullptr;
    WindowProbeBackendController *m_backendController = nullptr;
    WindowProbeOptions m_options;
    QTextStream *m_output = nullptr;
    QTextStream *m_error = nullptr;
    bool m_finished = false;
    bool m_initialSnapshotPublished = false;
    QTimer *m_startupTimer = nullptr;
};

void configureParser(QCommandLineParser &parser);
ParseOptionsResult parseOptions(const QCommandLineParser &parser);
std::optional<Command> parseCommand(const QString &value);
QByteArray formatJsonResultBytes(const WindowProbeOptions &options,
                                 const QString &backendName,
                                 const plasma_bridge::WindowSnapshot &snapshot);
QByteArray formatJsonResultBytes(const WindowProbeOptions &options, const WindowProbeCommandResult &result);
QString formatHumanResultText(const WindowProbeOptions &options,
                              const QString &backendName,
                              const plasma_bridge::WindowSnapshot &snapshot);
QString formatHumanResultText(const WindowProbeOptions &options, const WindowProbeCommandResult &result);

} // namespace plasma_bridge::tools::window_probe
