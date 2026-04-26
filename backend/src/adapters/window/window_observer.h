#pragma once

#include "common/window_state.h"

#include <QObject>
#include <QString>

namespace plasma_bridge::window
{

class WindowObserver : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~WindowObserver() override = default;

    virtual void start() = 0;
    virtual const plasma_bridge::WindowSnapshot &currentSnapshot() const = 0;
    virtual bool hasInitialSnapshot() const = 0;
    virtual QString backendName() const = 0;

signals:
    void initialSnapshotReady();
    void windowStateChanged(const QString &reason, const QString &windowId);
    void connectionFailed(const QString &message);
};

} // namespace plasma_bridge::window
