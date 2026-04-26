#pragma once

#include "adapters/window/window_observer.h"

#include <QObject>
#include <QString>

#include <memory>
#include <optional>

namespace plasma_bridge::window
{

struct KWinScriptBackendStatus {
    QString backendName;
    bool scriptInstalled = false;
    bool scriptEnabled = false;
    bool helperServiceInstalled = false;
    bool helperServiceRegistered = false;
    bool snapshotCached = false;
    bool snapshotReady = false;
};

struct KWinScriptBackendCommandResult {
    bool ok = false;
    QString message;
    KWinScriptBackendStatus status;
};

QString kwinScriptBackendName();
QString kwinScriptBackendReadinessError(const KWinScriptBackendStatus &status);
std::optional<plasma_bridge::WindowSnapshot> readKWinScriptCachedSnapshot(QString *errorMessage = nullptr);

class KWinScriptWindowBackendController final : public QObject
{
    Q_OBJECT

public:
    explicit KWinScriptWindowBackendController(QObject *parent = nullptr);
    ~KWinScriptWindowBackendController() override;

    KWinScriptBackendCommandResult setup();
    KWinScriptBackendCommandResult status() const;
    KWinScriptBackendCommandResult teardown();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

class KWinScriptWindowObserver final : public WindowObserver
{
    Q_OBJECT

public:
    explicit KWinScriptWindowObserver(QObject *parent = nullptr);
    ~KWinScriptWindowObserver() override;

    void start() override;
    const plasma_bridge::WindowSnapshot &currentSnapshot() const override;
    bool hasInitialSnapshot() const override;
    QString backendName() const override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace plasma_bridge::window
