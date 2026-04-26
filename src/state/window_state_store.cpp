#include "state/window_state_store.h"

#include "adapters/window/window_observer.h"

namespace plasma_bridge::state
{

WindowStateStore::WindowStateStore(QObject *parent)
    : QObject(parent)
{
}

void WindowStateStore::attachObserver(window::WindowObserver *observer)
{
    if (observer == nullptr) {
        return;
    }

    connect(observer, &window::WindowObserver::initialSnapshotReady, this, [this, observer]() {
        updateWindowState(observer->currentSnapshot(), observer->hasInitialSnapshot(), QStringLiteral("initial"));
    });
    connect(observer,
            &window::WindowObserver::windowStateChanged,
            this,
            [this, observer](const QString &reason, const QString &windowId) {
                updateWindowState(observer->currentSnapshot(), observer->hasInitialSnapshot(), reason, windowId);
            });

    if (observer->hasInitialSnapshot()) {
        updateWindowState(observer->currentSnapshot(), observer->hasInitialSnapshot(), QStringLiteral("initial"));
    }
}

void WindowStateStore::updateWindowState(const plasma_bridge::WindowSnapshot &snapshot,
                                         const bool ready,
                                         const QString &reason,
                                         const QString &windowId)
{
    const bool wasReady = m_ready;
    m_windowState = plasma_bridge::normalizeWindowSnapshot(snapshot);
    m_ready = ready;

    if (wasReady != m_ready) {
        emit readyChanged(m_ready);
    }

    emit windowStateChanged(reason, windowId);
}

bool WindowStateStore::isReady() const
{
    return m_ready;
}

const plasma_bridge::WindowSnapshot &WindowStateStore::windowState() const
{
    return m_windowState;
}

std::optional<plasma_bridge::WindowState> WindowStateStore::activeWindow() const
{
    if (!m_ready) {
        return std::nullopt;
    }

    return plasma_bridge::activeWindow(m_windowState);
}

} // namespace plasma_bridge::state
