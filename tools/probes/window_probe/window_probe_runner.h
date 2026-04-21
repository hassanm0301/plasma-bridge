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

class PlasmaWaylandWindowProbeSource final : public WindowProbeSource
{
    Q_OBJECT

public:
    explicit PlasmaWaylandWindowProbeSource(QObject *parent = nullptr);
    ~PlasmaWaylandWindowProbeSource() override;

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
                               const WindowProbeOptions &options,
                               QTextStream *output,
                               QTextStream *error,
                               QObject *parent = nullptr);

    void start();

signals:
    void finished(int exitCode);

private:
    void finish(int exitCode);
    void publishInitialSnapshot();
    void printSnapshot() const;

    WindowProbeSource *m_source = nullptr;
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
QString formatHumanResultText(const WindowProbeOptions &options,
                              const QString &backendName,
                              const plasma_bridge::WindowSnapshot &snapshot);

} // namespace plasma_bridge::tools::window_probe
