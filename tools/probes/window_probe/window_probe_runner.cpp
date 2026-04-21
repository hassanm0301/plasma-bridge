#include "tools/probes/window_probe/window_probe_runner.h"

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/registry.h>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QHash>
#include <QJsonDocument>
#include <QList>
#include <QPointer>
#include <QTextStream>
#include <QTimer>

namespace plasma_bridge::tools::window_probe
{
namespace
{

const QString kPlasmaWaylandBackendName = QStringLiteral("plasma-wayland");

QJsonValue stringOrNull(const QString &value)
{
    return value.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(value);
}

QString commandName(const Command command)
{
    switch (command) {
    case Command::List:
        return QStringLiteral("list");
    case Command::Active:
        return QStringLiteral("active");
    }

    return QStringLiteral("list");
}

plasma_bridge::WindowState toWindowState(KWayland::Client::PlasmaWindow *window)
{
    plasma_bridge::WindowState state;
    state.id = QString::fromUtf8(window->uuid());
    state.title = window->title();
    state.appId = window->appId();
    state.pid = window->pid();
    state.isActive = window->isActive();
    state.isMinimized = window->isMinimized();
    state.isMaximized = window->isMaximized();
    state.isFullscreen = window->isFullscreen();
    state.isOnAllDesktops = window->isOnAllDesktops();
    state.skipTaskbar = window->skipTaskbar();
    state.skipSwitcher = window->skipSwitcher();
    state.geometry = plasma_bridge::toWindowGeometryState(window->geometry());
    state.clientGeometry = plasma_bridge::toWindowGeometryState(window->clientGeometry());
    state.virtualDesktopIds = window->plasmaVirtualDesktops();
    state.activityIds = window->plasmaActivities();
    state.resourceName = window->resourceName();

    if (const QPointer<KWayland::Client::PlasmaWindow> parentWindow = window->parentWindow(); !parentWindow.isNull()) {
        state.parentId = QString::fromUtf8(parentWindow->uuid());
    }

    return state;
}

plasma_bridge::WindowSnapshot snapshotFromWindowManagement(KWayland::Client::PlasmaWindowManagement *windowManagement)
{
    plasma_bridge::WindowSnapshot snapshot;
    if (windowManagement == nullptr) {
        return snapshot;
    }

    QHash<QString, plasma_bridge::WindowState> windowsById;
    QList<QString> originalOrder;

    const QList<KWayland::Client::PlasmaWindow *> windows = windowManagement->windows();
    for (KWayland::Client::PlasmaWindow *window : windows) {
        const plasma_bridge::WindowState state = toWindowState(window);
        windowsById.insert(state.id, state);
        originalOrder.append(state.id);

        if (snapshot.activeWindowId.isEmpty() && state.isActive) {
            snapshot.activeWindowId = state.id;
        }
    }

    if (KWayland::Client::PlasmaWindow *activeWindow = windowManagement->activeWindow(); activeWindow != nullptr) {
        snapshot.activeWindowId = QString::fromUtf8(activeWindow->uuid());
    }

    const QList<QByteArray> stackingOrder = windowManagement->stackingOrderUuids();
    QList<QString> orderedIds;
    orderedIds.reserve(originalOrder.size());

    for (const QByteArray &windowIdBytes : stackingOrder) {
        const QString windowId = QString::fromUtf8(windowIdBytes);
        if (windowsById.contains(windowId) && !orderedIds.contains(windowId)) {
            orderedIds.append(windowId);
        }
    }

    for (const QString &windowId : originalOrder) {
        if (!orderedIds.contains(windowId)) {
            orderedIds.append(windowId);
        }
    }

    for (const QString &windowId : orderedIds) {
        snapshot.windows.append(windowsById.value(windowId));
    }

    if (const std::optional<plasma_bridge::WindowState> resolvedActiveWindow = plasma_bridge::activeWindow(snapshot);
        resolvedActiveWindow.has_value()) {
        snapshot.activeWindowId = resolvedActiveWindow->id;
    } else {
        snapshot.activeWindowId.clear();
    }

    return snapshot;
}

} // namespace

class PlasmaWaylandWindowProbeSource::Impl final : public QObject
{
public:
    explicit Impl(PlasmaWaylandWindowProbeSource *owner)
        : QObject(owner)
        , m_owner(owner)
    {
        connect(&m_connection, &KWayland::Client::ConnectionThread::connected, this, [this]() {
            m_registry.create(&m_connection);
            m_registry.setup();

            QTimer::singleShot(0, this, [this]() {
                m_connection.roundtrip();
                if (m_windowManagement == nullptr) {
                    emitFailure(QStringLiteral("Plasma Wayland window management interface is unavailable. "
                                               "Ensure this runs inside a KDE Plasma Wayland session."));
                    return;
                }

                m_connection.roundtrip();
                m_snapshot = snapshotFromWindowManagement(m_windowManagement);
                m_ready = true;
                emit m_owner->initialSnapshotReady();
            });
        });

        connect(&m_connection, &KWayland::Client::ConnectionThread::failed, this, [this]() {
            emitFailure(QStringLiteral("Failed to connect to the Wayland display at %1.")
                            .arg(m_connection.socketName().isEmpty() ? QStringLiteral("(default socket)")
                                                                     : m_connection.socketName()));
        });

        connect(&m_connection, &KWayland::Client::ConnectionThread::connectionDied, this, [this]() {
            emitFailure(QStringLiteral("Connection to the Wayland display was lost before the window snapshot was ready."));
        });

        connect(&m_connection, &KWayland::Client::ConnectionThread::errorOccurred, this, [this]() {
            emitFailure(QStringLiteral("Wayland connection error %1.").arg(m_connection.errorCode()));
        });

        connect(&m_registry, &KWayland::Client::Registry::plasmaWindowManagementAnnounced, this, [this](const quint32 name, const quint32 version) {
            if (m_windowManagement != nullptr) {
                return;
            }

            m_windowManagement = m_registry.createPlasmaWindowManagement(name, version, this);
            if (m_windowManagement == nullptr || !m_windowManagement->isValid()) {
                emitFailure(QStringLiteral("Failed to bind the Plasma Wayland window management interface."));
                return;
            }

            connect(m_windowManagement, &KWayland::Client::PlasmaWindowManagement::removed, this, [this]() {
                if (!m_ready) {
                    emitFailure(QStringLiteral("Plasma Wayland window management interface became unavailable."));
                }
            });
        });

        connect(&m_registry, &KWayland::Client::Registry::plasmaWindowManagementRemoved, this, [this](const quint32) {
            if (!m_ready) {
                emitFailure(QStringLiteral("Plasma Wayland window management interface became unavailable."));
            }
        });
    }

    void start()
    {
        if (m_started) {
            return;
        }
        m_started = true;

        if (qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
            emitFailure(QStringLiteral("window_probe requires a KDE Plasma Wayland session (WAYLAND_DISPLAY is not set)."));
            return;
        }

        const QString sessionType = qEnvironmentVariable("XDG_SESSION_TYPE");
        if (!sessionType.isEmpty() && sessionType.compare(QStringLiteral("wayland"), Qt::CaseInsensitive) != 0) {
            emitFailure(QStringLiteral("window_probe requires a KDE Plasma Wayland session."));
            return;
        }

        m_connection.initConnection();
    }

    const plasma_bridge::WindowSnapshot &currentSnapshot() const
    {
        return m_snapshot;
    }

    bool hasInitialSnapshot() const
    {
        return m_ready;
    }

    QString backendName() const
    {
        return kPlasmaWaylandBackendName;
    }

private:
    void emitFailure(const QString &message)
    {
        emit m_owner->connectionFailed(message);
    }

    PlasmaWaylandWindowProbeSource *m_owner = nullptr;
    KWayland::Client::ConnectionThread m_connection;
    KWayland::Client::Registry m_registry;
    KWayland::Client::PlasmaWindowManagement *m_windowManagement = nullptr;
    plasma_bridge::WindowSnapshot m_snapshot;
    bool m_started = false;
    bool m_ready = false;
};

PlasmaWaylandWindowProbeSource::PlasmaWaylandWindowProbeSource(QObject *parent)
    : WindowProbeSource(parent)
    , m_impl(std::make_unique<Impl>(this))
{
}

PlasmaWaylandWindowProbeSource::~PlasmaWaylandWindowProbeSource() = default;

void PlasmaWaylandWindowProbeSource::start()
{
    m_impl->start();
}

const plasma_bridge::WindowSnapshot &PlasmaWaylandWindowProbeSource::currentSnapshot() const
{
    return m_impl->currentSnapshot();
}

bool PlasmaWaylandWindowProbeSource::hasInitialSnapshot() const
{
    return m_impl->hasInitialSnapshot();
}

QString PlasmaWaylandWindowProbeSource::backendName() const
{
    return m_impl->backendName();
}

WindowProbeRunner::WindowProbeRunner(WindowProbeSource *source,
                                     const WindowProbeOptions &options,
                                     QTextStream *output,
                                     QTextStream *error,
                                     QObject *parent)
    : QObject(parent)
    , m_source(source)
    , m_options(options)
    , m_output(output)
    , m_error(error)
    , m_startupTimer(new QTimer(this))
{
    m_startupTimer->setSingleShot(true);

    connect(m_startupTimer, &QTimer::timeout, this, [this]() {
        if (m_source != nullptr && !m_source->hasInitialSnapshot()) {
            if (m_error != nullptr) {
                *m_error << "Timed out waiting for Plasma Wayland window state." << Qt::endl;
            }
            finish(1);
        }
    });

    if (m_source == nullptr) {
        return;
    }

    connect(m_source, &WindowProbeSource::connectionFailed, this, [this](const QString &message) {
        if (m_error != nullptr) {
            *m_error << message << Qt::endl;
        }
        if (m_source != nullptr && !m_source->hasInitialSnapshot()) {
            finish(1);
        }
    });

    connect(m_source, &WindowProbeSource::initialSnapshotReady, this, [this]() {
        publishInitialSnapshot();
    });
}

void WindowProbeRunner::start()
{
    if (m_finished || m_source == nullptr) {
        if (!m_finished) {
            finish(1);
        }
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
                                 QStringLiteral("One of: list, active."));
}

ParseOptionsResult parseOptions(const QCommandLineParser &parser)
{
    ParseOptionsResult result;

    const QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.size() != 1) {
        result.errorMessage = QStringLiteral("window_probe expects exactly one command: list or active.");
        return result;
    }

    const std::optional<Command> command = parseCommand(positionalArguments.constFirst());
    if (!command.has_value()) {
        result.errorMessage =
            QStringLiteral("Unsupported command: %1. Expected list or active.").arg(positionalArguments.constFirst());
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
        json[QStringLiteral("windows")] = snapshotJson.value(QStringLiteral("windows"));
    } else {
        const std::optional<plasma_bridge::WindowState> window = plasma_bridge::activeWindow(snapshot);
        json[QStringLiteral("window")] =
            window.has_value() ? QJsonValue(plasma_bridge::toJsonObject(*window)) : QJsonValue(QJsonValue::Null);
    }

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

} // namespace plasma_bridge::tools::window_probe
