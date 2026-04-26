#pragma once

#include "adapters/window/window_observer.h"

#include <QSet>
#include <QString>
#include <QTimer>

namespace KWayland::Client
{
class PlasmaWindow;
class PlasmaWindowManagement;
}

namespace plasma_bridge::window
{

class PlasmaWaylandWindowObserver final : public WindowObserver
{
    Q_OBJECT

public:
    explicit PlasmaWaylandWindowObserver(QObject *parent = nullptr);
    ~PlasmaWaylandWindowObserver() override;

    void start() override;

    const plasma_bridge::WindowSnapshot &currentSnapshot() const override;
    bool hasInitialSnapshot() const override;
    QString backendName() const override;

private:
    class Impl;
    Impl *m_impl = nullptr;
};

} // namespace plasma_bridge::window
