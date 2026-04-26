#include "adapters/window/plasma_wayland_window_observer.h"

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/registry.h>

#include <QHash>
#include <QList>
#include <QPointer>

#include <algorithm>

namespace plasma_bridge::window
{
namespace
{

const QString kPlasmaWaylandBackendName = QStringLiteral("plasma-wayland");
const QString kWindowManagementInterfaceName = QStringLiteral("org_kde_plasma_window_management");

plasma_bridge::WindowState toWindowState(KWayland::Client::PlasmaWindow *window)
{
    plasma_bridge::WindowState state;
    if (window == nullptr) {
        return state;
    }

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
        if (state.id.isEmpty()) {
            continue;
        }

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

    return plasma_bridge::normalizeWindowSnapshot(snapshot);
}

QString windowId(KWayland::Client::PlasmaWindow *window)
{
    return window == nullptr ? QString() : QString::fromUtf8(window->uuid());
}

QString connectionContext(KWayland::Client::ConnectionThread &connection)
{
    const QString socketName =
        connection.socketName().isEmpty() ? QStringLiteral("(default socket)") : connection.socketName();
    const QString sessionType = qEnvironmentVariable("XDG_SESSION_TYPE");
    const QString currentDesktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    const QString waylandDisplay = qEnvironmentVariable("WAYLAND_DISPLAY");

    return QStringLiteral("socket=%1, WAYLAND_DISPLAY=%2, XDG_SESSION_TYPE=%3, XDG_CURRENT_DESKTOP=%4")
        .arg(socketName,
             waylandDisplay.isEmpty() ? QStringLiteral("(unset)") : waylandDisplay,
             sessionType.isEmpty() ? QStringLiteral("(unset)") : sessionType,
             currentDesktop.isEmpty() ? QStringLiteral("(unset)") : currentDesktop);
}

} // namespace

class PlasmaWaylandWindowObserver::Impl final : public QObject
{
public:
    explicit Impl(PlasmaWaylandWindowObserver *owner)
        : QObject(owner)
        , m_owner(owner)
    {
        m_initialPublishTimer.setSingleShot(true);
        connect(&m_initialPublishTimer, &QTimer::timeout, this, [this]() {
            if (m_initialSnapshotPublished) {
                return;
            }

            if (m_windowManagement != nullptr) {
                for (KWayland::Client::PlasmaWindow *window : m_windowManagement->windows()) {
                    attachWindow(window);
                }
            }

            publishInitialSnapshot();
        });

        connect(&m_connection, &KWayland::Client::ConnectionThread::connected, this, [this]() {
            m_registry.create(&m_connection);
            m_registry.setup();

            QTimer::singleShot(0, this, [this]() {
                m_connection.roundtrip();
                if (m_windowManagement == nullptr) {
                    emitFailure(QStringLiteral("%1 was not announced by the compositor. "
                                               "The app connected to Wayland successfully, but KDE Plasma window state is unavailable on this session. "
                                               "This usually means the process is running outside the primary KWin user session, or the compositor did not expose that privileged protocol. "
                                               "(%2)")
                                    .arg(kWindowManagementInterfaceName, connectionContext(m_connection)));
                    return;
                }

                connectWindowManagementSignals();
                m_connection.roundtrip();
                for (KWayland::Client::PlasmaWindow *window : m_windowManagement->windows()) {
                    attachWindow(window);
                }
                publishInitialSnapshot();
            });
        });

        connect(&m_connection, &KWayland::Client::ConnectionThread::failed, this, [this]() {
            emitFailure(QStringLiteral("Failed to connect to the Wayland display at %1.")
                            .arg(m_connection.socketName().isEmpty() ? QStringLiteral("(default socket)")
                                                                     : m_connection.socketName()));
        });

        connect(&m_connection, &KWayland::Client::ConnectionThread::connectionDied, this, [this]() {
            emitFailure(QStringLiteral("Connection to the Wayland display was lost."));
        });

        connect(&m_connection, &KWayland::Client::ConnectionThread::errorOccurred, this, [this]() {
            emitFailure(QStringLiteral("Wayland connection error %1.").arg(m_connection.errorCode()));
        });

        connect(&m_registry,
                &KWayland::Client::Registry::plasmaWindowManagementAnnounced,
                this,
                [this](const quint32 name, const quint32 version) {
                    if (m_windowManagement != nullptr) {
                        return;
                    }

                    m_windowManagement = m_registry.createPlasmaWindowManagement(name, version, this);
                    if (m_windowManagement == nullptr || !m_windowManagement->isValid()) {
                        emitFailure(QStringLiteral("Failed to bind the Plasma Wayland window management interface."));
                        return;
                    }
                });

        connect(&m_registry, &KWayland::Client::Registry::plasmaWindowManagementRemoved, this, [this](const quint32) {
            emitFailure(QStringLiteral("Plasma Wayland window management interface became unavailable."));
        });
    }

    void start()
    {
        if (m_started) {
            return;
        }
        m_started = true;

        if (qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
            emitFailure(QStringLiteral("window state requires a KDE Plasma Wayland session (WAYLAND_DISPLAY is not set)."));
            return;
        }

        const QString sessionType = qEnvironmentVariable("XDG_SESSION_TYPE");
        if (!sessionType.isEmpty() && sessionType.compare(QStringLiteral("wayland"), Qt::CaseInsensitive) != 0) {
            emitFailure(QStringLiteral("window state requires a KDE Plasma Wayland session."));
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
        return m_initialSnapshotPublished;
    }

    QString backendName() const
    {
        return kPlasmaWaylandBackendName;
    }

private:
    void connectWindowManagementSignals()
    {
        if (m_windowManagement == nullptr || m_windowManagementSignalsConnected) {
            return;
        }
        m_windowManagementSignalsConnected = true;

        connect(m_windowManagement, &KWayland::Client::PlasmaWindowManagement::windowCreated, this, [this](KWayland::Client::PlasmaWindow *window) {
            attachWindow(window);
            scheduleOrEmit(QStringLiteral("window-added"), windowId(window));
        });
        connect(m_windowManagement, &KWayland::Client::PlasmaWindowManagement::activeWindowChanged, this, [this]() {
            scheduleOrEmit(QStringLiteral("active-window-changed"), windowId(m_windowManagement->activeWindow()));
        });
        connect(m_windowManagement, &KWayland::Client::PlasmaWindowManagement::stackingOrderUuidsChanged, this, [this]() {
            scheduleOrEmit(QStringLiteral("stacking-order-changed"), QString());
        });
        connect(m_windowManagement, &KWayland::Client::PlasmaWindowManagement::removed, this, [this]() {
            emitFailure(QStringLiteral("Plasma Wayland window management interface became unavailable."));
        });
    }

    void attachWindow(KWayland::Client::PlasmaWindow *window)
    {
        if (window == nullptr || m_attachedWindows.contains(window)) {
            return;
        }

        m_attachedWindows.insert(window);

        connect(window, &QObject::destroyed, this, [this, window]() {
            m_attachedWindows.remove(window);
        });

        const auto emitWindowUpdated = [this, window]() {
            scheduleOrEmit(QStringLiteral("window-updated"), windowId(window));
        };

        connect(window, &KWayland::Client::PlasmaWindow::titleChanged, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::appIdChanged, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::activeChanged, this, [this, window]() {
            scheduleOrEmit(QStringLiteral("active-window-changed"), windowId(window));
        });
        connect(window, &KWayland::Client::PlasmaWindow::fullscreenChanged, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::minimizedChanged, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::maximizedChanged, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::onAllDesktopsChanged, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::skipTaskbarChanged, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::skipSwitcherChanged, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::parentWindowChanged, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::geometryChanged, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::resourceNameChanged, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::clientGeometryChanged, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::plasmaVirtualDesktopEntered, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::plasmaVirtualDesktopLeft, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::plasmaActivityEntered, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::plasmaActivityLeft, this, emitWindowUpdated);
        connect(window, &KWayland::Client::PlasmaWindow::unmapped, this, [this, window]() {
            scheduleOrEmit(QStringLiteral("window-removed"), windowId(window));
        });
    }

    void scheduleOrEmit(const QString &reason, const QString &windowId)
    {
        if (!m_initialSnapshotPublished) {
            m_initialPublishTimer.start(200);
            return;
        }

        refreshAndEmit(reason, windowId);
    }

    void publishInitialSnapshot()
    {
        if (m_initialSnapshotPublished || m_windowManagement == nullptr) {
            return;
        }

        m_snapshot = snapshotFromWindowManagement(m_windowManagement);
        m_initialSnapshotPublished = true;
        m_initialPublishTimer.stop();
        emit m_owner->initialSnapshotReady();
    }

    void refreshAndEmit(const QString &reason, const QString &windowId)
    {
        m_snapshot = snapshotFromWindowManagement(m_windowManagement);
        emit m_owner->windowStateChanged(reason, windowId);
    }

    void emitFailure(const QString &message)
    {
        emit m_owner->connectionFailed(message);
    }

    PlasmaWaylandWindowObserver *m_owner = nullptr;
    KWayland::Client::ConnectionThread m_connection;
    KWayland::Client::Registry m_registry;
    KWayland::Client::PlasmaWindowManagement *m_windowManagement = nullptr;
    plasma_bridge::WindowSnapshot m_snapshot;
    QSet<const QObject *> m_attachedWindows;
    QTimer m_initialPublishTimer;
    bool m_started = false;
    bool m_initialSnapshotPublished = false;
    bool m_windowManagementSignalsConnected = false;
};

PlasmaWaylandWindowObserver::PlasmaWaylandWindowObserver(QObject *parent)
    : QObject(parent)
    , m_impl(new Impl(this))
{
}

PlasmaWaylandWindowObserver::~PlasmaWaylandWindowObserver() = default;

void PlasmaWaylandWindowObserver::start()
{
    m_impl->start();
}

const plasma_bridge::WindowSnapshot &PlasmaWaylandWindowObserver::currentSnapshot() const
{
    return m_impl->currentSnapshot();
}

bool PlasmaWaylandWindowObserver::hasInitialSnapshot() const
{
    return m_impl->hasInitialSnapshot();
}

QString PlasmaWaylandWindowObserver::backendName() const
{
    return m_impl->backendName();
}

} // namespace plasma_bridge::window
