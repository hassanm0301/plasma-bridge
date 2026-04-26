#pragma once

#include "common/window_state.h"

#include <QObject>

#include <optional>

namespace plasma_bridge::window
{
class WindowObserver;
}

namespace plasma_bridge::state
{

class WindowStateStore final : public QObject
{
    Q_OBJECT

public:
    explicit WindowStateStore(QObject *parent = nullptr);

    void attachObserver(window::WindowObserver *observer);
    void updateWindowState(const plasma_bridge::WindowSnapshot &snapshot,
                           bool ready,
                           const QString &reason,
                           const QString &windowId = {});

    bool isReady() const;
    const plasma_bridge::WindowSnapshot &windowState() const;
    std::optional<plasma_bridge::WindowState> activeWindow() const;

signals:
    void readyChanged(bool ready);
    void windowStateChanged(const QString &reason, const QString &windowId);

private:
    plasma_bridge::WindowSnapshot m_windowState;
    bool m_ready = false;
};

} // namespace plasma_bridge::state
