#include "tools/probes/window_probe/window_probe_runner.h"

#include "adapters/window/kwin_script_window_backend.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QTimer>

namespace plasma_bridge::tools::window_probe
{
namespace
{

constexpr int kSnapshotPollIntervalMs = 200;

QString commandName(const Command command)
{
    switch (command) {
    case Command::List:
        return QStringLiteral("list");
    case Command::Active:
        return QStringLiteral("active");
    case Command::Setup:
        return QStringLiteral("setup");
    case Command::Status:
        return QStringLiteral("status");
    case Command::Teardown:
        return QStringLiteral("teardown");
    }

    return QStringLiteral("list");
}

bool commandUsesSnapshotSource(const Command command)
{
    return command == Command::List || command == Command::Active;
}

WindowProbeBackendStatus toProbeStatus(const window::KWinScriptBackendStatus &status)
{
    WindowProbeBackendStatus mapped;
    mapped.backendName = status.backendName;
    mapped.scriptInstalled = status.scriptInstalled;
    mapped.scriptEnabled = status.scriptEnabled;
    mapped.helperServiceInstalled = status.helperServiceInstalled;
    mapped.helperServiceRegistered = status.helperServiceRegistered;
    mapped.snapshotCached = status.snapshotCached;
    mapped.snapshotReady = status.snapshotReady;
    return mapped;
}

window::KWinScriptBackendStatus toSharedStatus(const WindowProbeBackendStatus &status)
{
    window::KWinScriptBackendStatus mapped;
    mapped.backendName = status.backendName;
    mapped.scriptInstalled = status.scriptInstalled;
    mapped.scriptEnabled = status.scriptEnabled;
    mapped.helperServiceInstalled = status.helperServiceInstalled;
    mapped.helperServiceRegistered = status.helperServiceRegistered;
    mapped.snapshotCached = status.snapshotCached;
    mapped.snapshotReady = status.snapshotReady;
    return mapped;
}

WindowProbeCommandResult toProbeResult(const window::KWinScriptBackendCommandResult &result)
{
    WindowProbeCommandResult mapped;
    mapped.ok = result.ok;
    mapped.message = result.message;
    mapped.status = toProbeStatus(result.status);
    return mapped;
}

QJsonObject statusJson(const WindowProbeBackendStatus &status)
{
    QJsonObject json;
    json[QStringLiteral("backend")] = status.backendName;
    json[QStringLiteral("scriptInstalled")] = status.scriptInstalled;
    json[QStringLiteral("scriptEnabled")] = status.scriptEnabled;
    json[QStringLiteral("helperServiceInstalled")] = status.helperServiceInstalled;
    json[QStringLiteral("helperServiceRegistered")] = status.helperServiceRegistered;
    json[QStringLiteral("snapshotCached")] = status.snapshotCached;
    json[QStringLiteral("snapshotReady")] = status.snapshotReady;
    return json;
}

QString boolText(const bool value)
{
    return value ? QStringLiteral("yes") : QStringLiteral("no");
}

QString backendReadinessError(const WindowProbeBackendStatus &status)
{
    return window::kwinScriptBackendReadinessError(toSharedStatus(status));
}

} // namespace

class KWinScriptWindowProbeBackendController::Impl final
{
public:
    WindowProbeCommandResult setup()
    {
        return toProbeResult(m_backend.setup());
    }

    WindowProbeCommandResult status() const
    {
        return toProbeResult(m_backend.status());
    }

    WindowProbeCommandResult teardown()
    {
        return toProbeResult(m_backend.teardown());
    }

private:
    window::KWinScriptWindowBackendController m_backend;
};

KWinScriptWindowProbeBackendController::KWinScriptWindowProbeBackendController(QObject *parent)
    : WindowProbeBackendController(parent)
    , m_impl(std::make_unique<Impl>())
{
}

KWinScriptWindowProbeBackendController::~KWinScriptWindowProbeBackendController() = default;

WindowProbeCommandResult KWinScriptWindowProbeBackendController::setup()
{
    return m_impl->setup();
}

WindowProbeCommandResult KWinScriptWindowProbeBackendController::status()
{
    return m_impl->status();
}

WindowProbeCommandResult KWinScriptWindowProbeBackendController::teardown()
{
    return m_impl->teardown();
}

class KWinScriptWindowProbeSource::Impl final : public QObject
{
public:
    explicit Impl(KWinScriptWindowProbeSource *owner)
        : QObject(owner)
        , m_owner(owner)
    {
        m_pollTimer.setInterval(kSnapshotPollIntervalMs);
        connect(&m_pollTimer, &QTimer::timeout, this, &Impl::pollCache);
    }

    void start()
    {
        if (m_started) {
            return;
        }

        m_started = true;
        pollCache();
        if (!m_ready) {
            m_pollTimer.start();
        }
    }

    void pollCache()
    {
        QString errorMessage;
        const std::optional<plasma_bridge::WindowSnapshot> snapshot =
            window::readKWinScriptCachedSnapshot(&errorMessage);
        if (snapshot.has_value()) {
            m_snapshot = *snapshot;
            m_ready = true;
            m_pollTimer.stop();
            emit m_owner->initialSnapshotReady();
            return;
        }

        if (!errorMessage.isEmpty()) {
            m_pollTimer.stop();
            emit m_owner->connectionFailed(errorMessage);
        }
    }

    KWinScriptWindowProbeSource *m_owner = nullptr;
    plasma_bridge::WindowSnapshot m_snapshot;
    QTimer m_pollTimer;
    bool m_started = false;
    bool m_ready = false;
};

KWinScriptWindowProbeSource::KWinScriptWindowProbeSource(QObject *parent)
    : WindowProbeSource(parent)
    , m_impl(std::make_unique<Impl>(this))
{
}

KWinScriptWindowProbeSource::~KWinScriptWindowProbeSource() = default;

void KWinScriptWindowProbeSource::start()
{
    m_impl->start();
}

const plasma_bridge::WindowSnapshot &KWinScriptWindowProbeSource::currentSnapshot() const
{
    return m_impl->m_snapshot;
}

bool KWinScriptWindowProbeSource::hasInitialSnapshot() const
{
    return m_impl->m_ready;
}

QString KWinScriptWindowProbeSource::backendName() const
{
    return window::kwinScriptBackendName();
}

WindowProbeRunner::WindowProbeRunner(WindowProbeSource *source,
                                     WindowProbeBackendController *backendController,
                                     const WindowProbeOptions &options,
                                     QTextStream *output,
                                     QTextStream *error,
                                     QObject *parent)
    : QObject(parent)
    , m_source(source)
    , m_backendController(backendController)
    , m_options(options)
    , m_output(output)
    , m_error(error)
    , m_startupTimer(new QTimer(this))
{
    m_startupTimer->setSingleShot(true);

    connect(m_startupTimer, &QTimer::timeout, this, [this]() {
        if (m_source != nullptr && !m_source->hasInitialSnapshot()) {
            if (m_error != nullptr) {
                *m_error << "Timed out waiting for cached KWin script window state." << Qt::endl;
            }
            finish(1);
        }
    });

    if (m_source != nullptr) {
        connect(m_source, &WindowProbeSource::connectionFailed, this, [this](const QString &message) {
            if (m_error != nullptr) {
                *m_error << message << Qt::endl;
            }
            if (!m_source->hasInitialSnapshot()) {
                finish(1);
            }
        });

        connect(m_source, &WindowProbeSource::initialSnapshotReady, this, [this]() {
            publishInitialSnapshot();
        });
    }
}

void WindowProbeRunner::start()
{
    if (m_finished) {
        return;
    }

    if (!commandUsesSnapshotSource(m_options.command)) {
        executeBackendCommand();
        return;
    }

    if (m_backendController != nullptr) {
        const WindowProbeCommandResult statusResult = m_backendController->status();
        const QString readinessError = backendReadinessError(statusResult.status);
        if (!readinessError.isEmpty()) {
            if (m_error != nullptr) {
                *m_error << readinessError << Qt::endl;
            }
            finish(1);
            return;
        }
    }

    if (m_source == nullptr) {
        finish(1);
        return;
    }

    if (m_options.timeoutMs >= 0) {
        m_startupTimer->start(m_options.timeoutMs);
    }

    m_source->start();
    if (m_source->hasInitialSnapshot()) {
        publishInitialSnapshot();
    }
}

void WindowProbeRunner::executeBackendCommand()
{
    if (m_backendController == nullptr) {
        if (m_error != nullptr) {
            *m_error << "window_probe backend controller is unavailable." << Qt::endl;
        }
        finish(1);
        return;
    }

    WindowProbeCommandResult result;
    switch (m_options.command) {
    case Command::Setup:
        result = m_backendController->setup();
        break;
    case Command::Status:
        result = m_backendController->status();
        break;
    case Command::Teardown:
        result = m_backendController->teardown();
        break;
    case Command::List:
    case Command::Active:
        finish(1);
        return;
    }

    printBackendCommandResult(result);
    finish(result.ok ? 0 : 1);
}

void WindowProbeRunner::finish(const int exitCode)
{
    if (m_finished) {
        return;
    }

    m_finished = true;
    m_startupTimer->stop();
    emit finished(exitCode);
}

void WindowProbeRunner::publishInitialSnapshot()
{
    if (m_initialSnapshotPublished || m_source == nullptr || !m_source->hasInitialSnapshot()) {
        return;
    }

    m_initialSnapshotPublished = true;
    m_startupTimer->stop();
    printSnapshot();
    finish(0);
}

void WindowProbeRunner::printSnapshot() const
{
    if (m_output == nullptr || m_source == nullptr) {
        return;
    }

    if (m_options.jsonOutput) {
        *m_output << formatJsonResultBytes(m_options, m_source->backendName(), m_source->currentSnapshot()) << Qt::endl;
        return;
    }

    *m_output << formatHumanResultText(m_options, m_source->backendName(), m_source->currentSnapshot()) << Qt::endl;
}

void WindowProbeRunner::printBackendCommandResult(const WindowProbeCommandResult &result) const
{
    if (m_output == nullptr) {
        return;
    }

    if (m_options.jsonOutput) {
        *m_output << formatJsonResultBytes(m_options, result) << Qt::endl;
        return;
    }

    *m_output << formatHumanResultText(m_options, result) << Qt::endl;
}

void configureParser(QCommandLineParser &parser)
{
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption(QStringLiteral("json"),
                                        QStringLiteral("Print machine-readable JSON output.")));
    parser.addOption(QCommandLineOption(QStringLiteral("timeout-ms"),
                                        QStringLiteral("Startup timeout before failing."),
                                        QStringLiteral("timeout-ms"),
                                        QString::number(PLASMA_BRIDGE_DEFAULT_PROBE_STARTUP_TIMEOUT_MS)));
    parser.addPositionalArgument(QStringLiteral("command"),
                                 QStringLiteral("One of: list, active, setup, status, teardown."));
}

ParseOptionsResult parseOptions(const QCommandLineParser &parser)
{
    ParseOptionsResult result;

    const QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.size() != 1) {
        result.errorMessage = QStringLiteral("window_probe expects exactly one command: list, active, setup, status, or teardown.");
        return result;
    }

    const std::optional<Command> command = parseCommand(positionalArguments.constFirst());
    if (!command.has_value()) {
        result.errorMessage = QStringLiteral("Unsupported command: %1. Expected list, active, setup, status, or teardown.")
                                  .arg(positionalArguments.constFirst());
        return result;
    }

    bool timeoutOk = false;
    const int timeoutMs = parser.value(QStringLiteral("timeout-ms")).toInt(&timeoutOk);
    if (!timeoutOk || timeoutMs < 0) {
        result.errorMessage = QStringLiteral("timeout-ms must be a non-negative integer.");
        return result;
    }

    WindowProbeOptions options;
    options.command = *command;
    options.timeoutMs = timeoutMs;
    options.jsonOutput = parser.isSet(QStringLiteral("json"));

    result.ok = true;
    result.options = options;
    return result;
}

std::optional<Command> parseCommand(const QString &value)
{
    if (value == QStringLiteral("list")) {
        return Command::List;
    }
    if (value == QStringLiteral("active")) {
        return Command::Active;
    }
    if (value == QStringLiteral("setup")) {
        return Command::Setup;
    }
    if (value == QStringLiteral("status")) {
        return Command::Status;
    }
    if (value == QStringLiteral("teardown")) {
        return Command::Teardown;
    }

    return std::nullopt;
}

QByteArray formatJsonResultBytes(const WindowProbeOptions &options,
                                 const QString &backendName,
                                 const plasma_bridge::WindowSnapshot &snapshot)
{
    QJsonObject json;
    json[QStringLiteral("command")] = commandName(options.command);
    json[QStringLiteral("backend")] = backendName;

    if (options.command == Command::List) {
        const QJsonObject snapshotJson = plasma_bridge::toJsonObject(snapshot);
        json[QStringLiteral("activeWindowId")] = snapshotJson.value(QStringLiteral("activeWindowId"));
        json[QStringLiteral("activeWindow")] = snapshotJson.value(QStringLiteral("activeWindow"));
        json[QStringLiteral("windows")] = snapshotJson.value(QStringLiteral("windows"));
    } else {
        const std::optional<plasma_bridge::WindowState> window = plasma_bridge::activeWindow(snapshot);
        json[QStringLiteral("window")] =
            window.has_value() ? QJsonValue(plasma_bridge::toJsonObject(*window)) : QJsonValue(QJsonValue::Null);
    }

    return QJsonDocument(json).toJson(QJsonDocument::Indented);
}

QByteArray formatJsonResultBytes(const WindowProbeOptions &options, const WindowProbeCommandResult &result)
{
    QJsonObject json;
    json[QStringLiteral("command")] = commandName(options.command);
    json[QStringLiteral("ok")] = result.ok;
    json[QStringLiteral("message")] = result.message;
    json[QStringLiteral("status")] = statusJson(result.status);
    return QJsonDocument(json).toJson(QJsonDocument::Indented);
}

QString formatHumanResultText(const WindowProbeOptions &options,
                              const QString &backendName,
                              const plasma_bridge::WindowSnapshot &snapshot)
{
    QString text;
    QTextStream stream(&text);
    stream << "Command: " << commandName(options.command) << '\n';
    stream << "Backend: " << backendName << '\n';

    if (options.command == Command::List) {
        stream << plasma_bridge::formatHumanReadableWindowList(snapshot);
    } else {
        stream << plasma_bridge::formatHumanReadableActiveWindow(plasma_bridge::activeWindow(snapshot));
    }

    return text;
}

QString formatHumanResultText(const WindowProbeOptions &options, const WindowProbeCommandResult &result)
{
    QString text;
    QTextStream stream(&text);
    stream << "Command: " << commandName(options.command) << '\n';
    stream << "Backend: " << result.status.backendName << '\n';
    stream << "OK: " << boolText(result.ok) << '\n';
    stream << "Message: " << result.message << '\n';
    stream << "Script Installed: " << boolText(result.status.scriptInstalled) << '\n';
    stream << "Script Enabled: " << boolText(result.status.scriptEnabled) << '\n';
    stream << "Helper Service Installed: " << boolText(result.status.helperServiceInstalled) << '\n';
    stream << "Helper Service Registered: " << boolText(result.status.helperServiceRegistered) << '\n';
    stream << "Snapshot Cached: " << boolText(result.status.snapshotCached) << '\n';
    stream << "Snapshot Ready: " << boolText(result.status.snapshotReady) << '\n';
    return text;
}

} // namespace plasma_bridge::tools::window_probe
