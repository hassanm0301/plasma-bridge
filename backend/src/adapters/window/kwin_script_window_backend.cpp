#include "adapters/window/kwin_script_window_backend.h"

#include "plasma_bridge_build_config.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>

namespace plasma_bridge::window
{
namespace
{

const QString kKWinScriptBackendName = QStringLiteral("kwin-script-helper");
const QString kKWinServiceName = QStringLiteral("org.kde.KWin");
const QString kKWinObjectPath = QStringLiteral("/KWin");
const QString kKWinInterfaceName = QStringLiteral("org.kde.KWin");
const QString kCacheDirectoryName = QStringLiteral("plasma-bridge/window_probe");
const QString kSnapshotFileName = QStringLiteral("snapshot.json");
const QString kServiceDirectoryName = QStringLiteral("dbus-1/services");
const QString kServiceFileName = QStringLiteral("org.plasma_remote_toolbar.WindowProbe.service");
const QString kScriptMetadataFileName = QStringLiteral("metadata.json");
const QString kScriptMainFileName = QStringLiteral("main.js");
const QString kScriptCodeDirectoryName = QStringLiteral("contents/code");

QString genericDataRoot()
{
    const QString override = qEnvironmentVariable("PLASMA_BRIDGE_WINDOW_PROBE_DATA_ROOT");
    return override.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) : override;
}

QString configRoot()
{
    const QString override = qEnvironmentVariable("PLASMA_BRIDGE_WINDOW_PROBE_CONFIG_ROOT");
    return override.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) : override;
}

QString cacheRootPath()
{
    return QDir(genericDataRoot()).filePath(kCacheDirectoryName);
}

QString snapshotFilePath()
{
    return QDir(cacheRootPath()).filePath(kSnapshotFileName);
}

QString helperServiceDirectoryPath()
{
    return QDir(genericDataRoot()).filePath(kServiceDirectoryName);
}

QString helperServiceFilePath()
{
    return QDir(helperServiceDirectoryPath()).filePath(kServiceFileName);
}

QString kwinScriptDirectoryPath()
{
    return QDir(genericDataRoot()).filePath(QStringLiteral("kwin/scripts/%1").arg(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_SCRIPT_ID)));
}

QString kwinScriptMainFilePath()
{
    return QDir(kwinScriptDirectoryPath()).filePath(QStringLiteral("%1/%2").arg(kScriptCodeDirectoryName, kScriptMainFileName));
}

QString kwinScriptMetadataFilePath()
{
    return QDir(kwinScriptDirectoryPath()).filePath(kScriptMetadataFileName);
}

QString kwinConfigFilePath()
{
    return QDir(configRoot()).filePath(QStringLiteral("kwinrc"));
}

bool isExecutableFile(const QString &path)
{
    const QFileInfo info(path);
    return info.exists() && info.isFile() && info.isExecutable();
}

QString resolvedHelperExecutablePath()
{
    const QString override = qEnvironmentVariable("PLASMA_BRIDGE_WINDOW_PROBE_HELPER_PATH");
    if (!override.isEmpty() && isExecutableFile(override)) {
        return override;
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    const QString sameDirectory = QDir(appDir).filePath(QStringLiteral("window_probe_helper"));
    if (isExecutableFile(sameDirectory)) {
        return sameDirectory;
    }

    const QString buildTreeFallback =
        QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("../../tools/probes/window_probe/window_probe_helper")));
    if (isExecutableFile(buildTreeFallback)) {
        return buildTreeFallback;
    }

    const QString pathExecutable = QStandardPaths::findExecutable(QStringLiteral("window_probe_helper"));
    if (!pathExecutable.isEmpty()) {
        return pathExecutable;
    }

    return override.isEmpty() ? sameDirectory : override;
}

QString serviceFileContents(const QString &execPath)
{
    return QStringLiteral("[D-BUS Service]\nName=%1\nExec=%2\n")
        .arg(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_SERVICE), execPath);
}

QString scriptMetadataContents()
{
    return QStringLiteral(
        "{\n"
        "    \"KPackageStructure\": \"KWin/Script\",\n"
        "    \"KPlugin\": {\n"
        "        \"Authors\": [\n"
        "            {\n"
        "                \"Email\": \"noreply@example.invalid\",\n"
        "                \"Name\": \"Plasma Bridge\"\n"
        "            }\n"
        "        ],\n"
        "        \"Description\": \"Publishes window snapshots for Plasma Bridge.\",\n"
        "        \"Icon\": \"preferences-system-windows-script-test\",\n"
        "        \"Id\": \"%1\",\n"
        "        \"License\": \"MIT\",\n"
        "        \"Name\": \"Plasma Bridge Window Backend\"\n"
        "    },\n"
        "    \"X-Plasma-API\": \"javascript\"\n"
        "}\n")
        .arg(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_SCRIPT_ID));
}

QString scriptMainContents()
{
    return QStringLiteral(R"JS(
function rectToObject(rect) {
    if (!rect) {
        return { x: 0, y: 0, width: 0, height: 0 };
    }

    return {
        x: Math.round(rect.x),
        y: Math.round(rect.y),
        width: Math.round(rect.width),
        height: Math.round(rect.height)
    };
}

function desktopIds(window) {
    if (!window || !window.desktops) {
        return [];
    }

    return window.desktops.map((desktop) => desktop ? String(desktop.id) : "").filter((id) => id.length > 0);
}

function activityIds(window) {
    if (!window || !window.activities) {
        return [];
    }

    return window.activities.map((activityId) => String(activityId));
}

function windowId(window) {
    return window && window.internalId ? window.internalId.toString() : "";
}

function windowState(window, activeWindowId) {
    let parentId = null;
    if (window && window.transientFor) {
        parentId = window.transientFor.internalId.toString();
    }

    return {
        id: windowId(window),
        title: window.caption || "",
        appId: window.desktopFileName || window.resourceClass || null,
        pid: window.pid || null,
        isActive: windowId(window) === activeWindowId,
        isMinimized: !!window.minimized,
        isMaximized: false,
        isFullscreen: !!window.fullScreen,
        isOnAllDesktops: desktopIds(window).length === 0,
        skipTaskbar: !!window.skipTaskbar,
        skipSwitcher: !!window.skipSwitcher,
        geometry: rectToObject(window.frameGeometry),
        clientGeometry: rectToObject(window.clientGeometry),
        virtualDesktopIds: desktopIds(window),
        activityIds: activityIds(window),
        parentId: parentId,
        resourceName: window.resourceName || null
    };
}

function trackedWindows() {
    return workspace.windowList().filter((window) => {
        return window && window.managed && !window.deleted && !window.desktopWindow && !window.dock;
    }).sort((left, right) => left.stackingOrder - right.stackingOrder);
}

function currentSnapshot() {
    const windows = trackedWindows();
    const activeWindowId = workspace.activeWindow ? windowId(workspace.activeWindow) : "";

    return {
        activeWindowId: activeWindowId.length > 0 ? activeWindowId : null,
        windows: windows.map((window) => windowState(window, activeWindowId))
    };
}

function publishSnapshot(reason, changedWindowId) {
    callDBus("%1",
             "%2",
             "%3",
             "PushSnapshot",
             "%4",
             JSON.stringify(currentSnapshot()),
             reason || "window-updated",
             changedWindowId || "");
}

let activationPollTimer = null;
let activationPollInFlight = false;

function findTrackedWindowById(targetWindowId) {
    const targetId = String(targetWindowId || "");
    const windows = trackedWindows();
    for (let index = 0; index < windows.length; ++index) {
        if (windowId(windows[index]) === targetId) {
            return windows[index];
        }
    }

    return null;
}

function ensureWindowVisibleForActivation(window) {
    if (!window) {
        return;
    }

    if (window.activities && window.activities.length > 0) {
        const activityId = String(window.activities[0]);
        if (activityId.length > 0 && workspace.currentActivity !== activityId) {
            workspace.currentActivity = activityId;
        }
    }

    if (window.desktops && window.desktops.length > 0) {
        workspace.currentDesktop = window.desktops[0];
    }

    if (window.minimized) {
        window.minimized = false;
    }
}

function activateWindowById(targetWindowId) {
    const targetWindow = findTrackedWindowById(targetWindowId);
    if (!targetWindow) {
        publishSnapshot("window-activation-failed", targetWindowId);
        return false;
    }

    ensureWindowVisibleForActivation(targetWindow);
    if (typeof workspace.raiseWindow === "function") {
        workspace.raiseWindow(targetWindow);
    }
    workspace.activeWindow = targetWindow;
    publishSnapshot("window-activation-requested", targetWindowId);
    return true;
}

function pollActivationRequest() {
    if (activationPollInFlight) {
        return;
    }

    activationPollInFlight = true;
    callDBus("%1",
             "%2",
             "%3",
             "TakeActivationRequest",
             function(targetWindowId) {
                 activationPollInFlight = false;
                 if (targetWindowId && String(targetWindowId).length > 0) {
                     activateWindowById(String(targetWindowId));
                     pollActivationRequest();
                 }
             });
}

function startActivationRequestPolling() {
    if (activationPollTimer) {
        return;
    }

    activationPollTimer = new QTimer();
    activationPollTimer.interval = 100;
    activationPollTimer.timeout.connect(pollActivationRequest);
    activationPollTimer.start();
    pollActivationRequest();
}

const watchedWindows = new Map();

function connectSignal(object, signalName, callback) {
    if (object && object[signalName] && object[signalName].connect) {
        object[signalName].connect(callback);
    }
}

function attachWindow(window) {
    if (!window || watchedWindows.has(window)) {
        return;
    }

    watchedWindows.set(window, true);

    const publishUpdated = () => publishSnapshot("window-updated", windowId(window));
    const publishActive = () => publishSnapshot("active-window-changed", windowId(window));

    connectSignal(window, "closed", () => {
        const id = windowId(window);
        watchedWindows.delete(window);
        publishSnapshot("window-removed", id);
    });
    connectSignal(window, "outputChanged", publishUpdated);
    connectSignal(window, "windowRoleChanged", publishUpdated);
    connectSignal(window, "windowClassChanged", publishUpdated);
    connectSignal(window, "frameGeometryChanged", publishUpdated);
    connectSignal(window, "clientGeometryChanged", publishUpdated);
    connectSignal(window, "fullScreenChanged", publishUpdated);
    connectSignal(window, "skipTaskbarChanged", publishUpdated);
    connectSignal(window, "skipSwitcherChanged", publishUpdated);
    connectSignal(window, "activeChanged", publishActive);
    connectSignal(window, "desktopsChanged", publishUpdated);
    connectSignal(window, "activitiesChanged", publishUpdated);
    connectSignal(window, "minimizedChanged", publishUpdated);
    connectSignal(window, "captionChanged", publishUpdated);
    connectSignal(window, "maximizedChanged", publishUpdated);
    connectSignal(window, "transientChanged", publishUpdated);
    connectSignal(window, "desktopFileNameChanged", publishUpdated);
    connectSignal(window, "stackingOrderChanged", () => publishSnapshot("stacking-order-changed", windowId(window)));
}

function attachExistingWindows() {
    trackedWindows().forEach((window) => attachWindow(window));
}

function main() {
    attachExistingWindows();
    publishSnapshot("initial", "");
    startActivationRequestPolling();

    connectSignal(workspace, "windowAdded", (window) => {
        attachWindow(window);
        publishSnapshot("window-added", windowId(window));
    });
    connectSignal(workspace, "windowRemoved", (window) => publishSnapshot("window-removed", windowId(window)));
    connectSignal(workspace, "windowActivated", (window) => publishSnapshot("active-window-changed", windowId(window)));
    connectSignal(workspace, "desktopsChanged", () => publishSnapshot("window-updated", ""));
    connectSignal(workspace, "activitiesChanged", () => publishSnapshot("window-updated", ""));
    connectSignal(workspace, "currentActivityChanged", () => publishSnapshot("window-updated", ""));
    connectSignal(workspace, "screensChanged", () => publishSnapshot("stacking-order-changed", ""));
    connectSignal(workspace, "stackingOrderChanged", () => publishSnapshot("stacking-order-changed", ""));
}

main();
)JS")
        .arg(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_SERVICE),
             QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_PATH),
             QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_INTERFACE),
             kKWinScriptBackendName);
}

bool ensureDirectory(const QString &path, QString *errorMessage)
{
    QDir dir;
    if (dir.mkpath(path)) {
        return true;
    }

    if (errorMessage != nullptr) {
        *errorMessage = QStringLiteral("Failed to create directory: %1").arg(path);
    }
    return false;
}

bool writeFileAtomically(const QString &path, const QString &contents, QString *errorMessage)
{
    const QFileInfo fileInfo(path);
    if (!ensureDirectory(fileInfo.dir().absolutePath(), errorMessage)) {
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to open %1 for writing: %2").arg(path, file.errorString());
        }
        return false;
    }

    if (file.write(contents.toUtf8()) < 0) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to write %1: %2").arg(path, file.errorString());
        }
        return false;
    }

    if (!file.commit()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to finalize %1: %2").arg(path, file.errorString());
        }
        return false;
    }

    return true;
}

bool removePath(const QString &path, QString *errorMessage)
{
    QFileInfo info(path);
    if (!info.exists()) {
        return true;
    }

    if (info.isDir()) {
        QDir dir(path);
        if (dir.removeRecursively()) {
            return true;
        }
    } else if (QFile::remove(path)) {
        return true;
    }

    if (errorMessage != nullptr) {
        *errorMessage = QStringLiteral("Failed to remove %1.").arg(path);
    }
    return false;
}

bool isScriptEnabled()
{
    QSettings settings(kwinConfigFilePath(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Plugins"));
    const bool enabled = settings.value(QStringLiteral("%1Enabled").arg(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_SCRIPT_ID)),
                                        false)
                             .toBool();
    settings.endGroup();
    return enabled;
}

void setScriptEnabled(const bool enabled)
{
    QSettings settings(kwinConfigFilePath(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Plugins"));
    settings.setValue(QStringLiteral("%1Enabled").arg(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_SCRIPT_ID)), enabled);
    settings.endGroup();
    settings.sync();
}

bool reconfigureKWin(QString *errorMessage)
{
    QDBusInterface kwin(kKWinServiceName, kKWinObjectPath, kKWinInterfaceName, QDBusConnection::sessionBus());
    if (!kwin.isValid()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to connect to the KWin DBus interface.");
        }
        return false;
    }

    const QDBusMessage reply = kwin.call(QStringLiteral("reconfigure"));
    if (reply.type() == QDBusMessage::ErrorMessage) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to ask KWin to reconfigure: %1").arg(reply.errorMessage());
        }
        return false;
    }

    return true;
}

bool stopHelperServiceBestEffort()
{
    QDBusInterface helper(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_SERVICE),
                          QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_PATH),
                          QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_INTERFACE),
                          QDBusConnection::sessionBus());
    if (!helper.isValid()) {
        return true;
    }

    helper.call(QStringLiteral("Shutdown"));
    return true;
}

std::optional<WindowSnapshot> snapshotFromJsonText(const QString &snapshotJson, QString *errorMessage = nullptr)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(snapshotJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Window snapshot JSON is invalid.");
        }
        return std::nullopt;
    }

    const std::optional<WindowSnapshot> snapshot = windowSnapshotFromJson(document.object());
    if (!snapshot.has_value() && errorMessage != nullptr) {
        *errorMessage = QStringLiteral("Window snapshot has an invalid shape.");
    }
    return snapshot;
}

KWinScriptBackendStatus computeBackendStatus()
{
    KWinScriptBackendStatus status;
    status.backendName = kKWinScriptBackendName;
    status.scriptInstalled = QFileInfo::exists(kwinScriptMetadataFilePath()) && QFileInfo::exists(kwinScriptMainFilePath());
    status.scriptEnabled = isScriptEnabled();
    status.helperServiceInstalled = QFileInfo::exists(helperServiceFilePath());

    if (QDBusConnectionInterface *interface = QDBusConnection::sessionBus().interface(); interface != nullptr) {
        const QDBusReply<bool> registered =
            interface->isServiceRegistered(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_SERVICE));
        status.helperServiceRegistered = registered.isValid() && registered.value();
    }

    status.snapshotCached = QFileInfo::exists(snapshotFilePath());
    status.snapshotReady = readKWinScriptCachedSnapshot().has_value();
    return status;
}

} // namespace

QString kwinScriptBackendName()
{
    return kKWinScriptBackendName;
}

QString kwinScriptBackendReadinessError(const KWinScriptBackendStatus &status)
{
    if (!status.scriptInstalled || !status.helperServiceInstalled || !status.scriptEnabled) {
        return QStringLiteral("window_probe backend is not configured. Run `window_probe setup` from a KDE Plasma session.");
    }

    return QString();
}

std::optional<WindowSnapshot> readKWinScriptCachedSnapshot(QString *errorMessage)
{
    QFile file(snapshotFilePath());
    if (!file.exists()) {
        return std::nullopt;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to open cached window snapshot: %1").arg(file.errorString());
        }
        return std::nullopt;
    }

    QString parseError;
    const std::optional<WindowSnapshot> snapshot = snapshotFromJsonText(QString::fromUtf8(file.readAll()), &parseError);
    if (!snapshot.has_value() && errorMessage != nullptr) {
        *errorMessage = parseError.isEmpty() ? QStringLiteral("Cached window snapshot is invalid.") : parseError;
    }
    return snapshot;
}

class KWinScriptWindowBackendController::Impl final
{
public:
    KWinScriptBackendCommandResult setup()
    {
        KWinScriptBackendCommandResult result;
        result.status.backendName = kKWinScriptBackendName;

        const QString helperPath = resolvedHelperExecutablePath();
        if (!isExecutableFile(helperPath)) {
            result.message =
                QStringLiteral("window_probe helper executable is missing at %1. Rebuild the probe tools first.").arg(helperPath);
            result.status = computeBackendStatus();
            return result;
        }

        QString errorMessage;
        if (!ensureDirectory(QDir(kwinScriptDirectoryPath()).filePath(kScriptCodeDirectoryName), &errorMessage)
            || !writeFileAtomically(kwinScriptMetadataFilePath(), scriptMetadataContents(), &errorMessage)
            || !writeFileAtomically(kwinScriptMainFilePath(), scriptMainContents(), &errorMessage)
            || !writeFileAtomically(helperServiceFilePath(), serviceFileContents(helperPath), &errorMessage)) {
            result.message = errorMessage;
            result.status = computeBackendStatus();
            return result;
        }

        setScriptEnabled(true);
        if (!reconfigureKWin(&errorMessage)) {
            result.message = errorMessage;
            result.status = computeBackendStatus();
            return result;
        }

        result.ok = true;
        result.message = QStringLiteral("Installed and enabled the KWin script backend for window state.");
        result.status = computeBackendStatus();
        return result;
    }

    KWinScriptBackendCommandResult status() const
    {
        KWinScriptBackendCommandResult result;
        result.ok = true;
        result.message = QStringLiteral("Collected KWin script backend status.");
        result.status = computeBackendStatus();
        return result;
    }

    KWinScriptBackendCommandResult teardown()
    {
        KWinScriptBackendCommandResult result;
        result.status.backendName = kKWinScriptBackendName;

        QString errorMessage;
        setScriptEnabled(false);
        stopHelperServiceBestEffort();

        if (!removePath(kwinScriptDirectoryPath(), &errorMessage)
            || !removePath(helperServiceFilePath(), &errorMessage)
            || !removePath(snapshotFilePath(), &errorMessage)) {
            result.message = errorMessage;
            result.status = computeBackendStatus();
            return result;
        }

        if (!reconfigureKWin(&errorMessage)) {
            result.message = errorMessage;
            result.status = computeBackendStatus();
            return result;
        }

        result.ok = true;
        result.message = QStringLiteral("Removed the KWin script backend for window state.");
        result.status = computeBackendStatus();
        return result;
    }
};

KWinScriptWindowBackendController::KWinScriptWindowBackendController(QObject *parent)
    : QObject(parent)
    , m_impl(std::make_unique<Impl>())
{
}

KWinScriptWindowBackendController::~KWinScriptWindowBackendController() = default;

KWinScriptBackendCommandResult KWinScriptWindowBackendController::setup()
{
    return m_impl->setup();
}

KWinScriptBackendCommandResult KWinScriptWindowBackendController::status() const
{
    return m_impl->status();
}

KWinScriptBackendCommandResult KWinScriptWindowBackendController::teardown()
{
    return m_impl->teardown();
}

class KWinScriptWindowActivationController::Impl final
{
public:
    control::WindowActivationResult activateWindow(const QString &windowId)
    {
        control::WindowActivationResult result;
        result.windowId = windowId;

        if (windowId.isEmpty()) {
            result.status = control::WindowActivationStatus::WindowNotFound;
            return result;
        }

        QDBusInterface helper(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_SERVICE),
                              QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_PATH),
                              QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_INTERFACE),
                              QDBusConnection::sessionBus());
        if (!helper.isValid()) {
            result.status = control::WindowActivationStatus::NotReady;
            return result;
        }

        const QDBusReply<bool> readyReply = helper.call(QStringLiteral("IsReady"));
        if (!readyReply.isValid() || !readyReply.value()) {
            result.status = control::WindowActivationStatus::NotReady;
            return result;
        }

        const QDBusReply<bool> reply = helper.call(QStringLiteral("RequestActivateWindow"), windowId);
        if (!reply.isValid()) {
            result.status = control::WindowActivationStatus::NotReady;
            return result;
        }

        result.status = reply.value() ? control::WindowActivationStatus::Accepted
                                      : control::WindowActivationStatus::WindowNotActivatable;
        return result;
    }
};

KWinScriptWindowActivationController::KWinScriptWindowActivationController()
    : m_impl(std::make_unique<Impl>())
{
}

KWinScriptWindowActivationController::~KWinScriptWindowActivationController() = default;

control::WindowActivationResult KWinScriptWindowActivationController::activateWindow(const QString &windowId)
{
    return m_impl->activateWindow(windowId);
}

class KWinScriptWindowObserver::Impl final : public QObject
{
    Q_OBJECT

public:
    explicit Impl(KWinScriptWindowObserver *owner)
        : QObject(owner)
        , m_owner(owner)
        , m_controller(this)
    {
    }

    void start()
    {
        if (m_started) {
            return;
        }
        m_started = true;

        const KWinScriptBackendCommandResult setupResult = m_controller.setup();
        if (!setupResult.ok) {
            emit m_owner->connectionFailed(setupResult.message);
            return;
        }

        if (!QDBusConnection::sessionBus().connect(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_SERVICE),
                                                   QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_PATH),
                                                   QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_INTERFACE),
                                                   QStringLiteral("SnapshotChanged"),
                                                   this,
                                                   SLOT(handleSnapshotChanged(QString, QString, QString, QString)))) {
            emit m_owner->connectionFailed(QStringLiteral("Failed to subscribe to KWin script window snapshot updates."));
            return;
        }

        publishCachedSnapshotIfAvailable();
        publishHelperSnapshotIfAvailable();
    }

    void publishCachedSnapshotIfAvailable()
    {
        QString errorMessage;
        const std::optional<WindowSnapshot> snapshot = readKWinScriptCachedSnapshot(&errorMessage);
        if (snapshot.has_value()) {
            publishSnapshot(*snapshot, QStringLiteral("initial"), QString());
        } else if (!errorMessage.isEmpty()) {
            emit m_owner->connectionFailed(errorMessage);
        }
    }

    void publishHelperSnapshotIfAvailable()
    {
        if (m_ready) {
            return;
        }

        QDBusInterface helper(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_SERVICE),
                              QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_PATH),
                              QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_INTERFACE),
                              QDBusConnection::sessionBus());
        if (!helper.isValid()) {
            return;
        }

        const QDBusReply<QString> reply = helper.call(QStringLiteral("GetSnapshot"));
        if (!reply.isValid() || reply.value().isEmpty()) {
            return;
        }

        QString errorMessage;
        const std::optional<WindowSnapshot> snapshot = snapshotFromJsonText(reply.value(), &errorMessage);
        if (snapshot.has_value()) {
            publishSnapshot(*snapshot, QStringLiteral("initial"), QString());
        } else if (!errorMessage.isEmpty()) {
            emit m_owner->connectionFailed(errorMessage);
        }
    }

    const WindowSnapshot &currentSnapshot() const
    {
        return m_snapshot;
    }

    bool hasInitialSnapshot() const
    {
        return m_ready;
    }

public slots:
    void handleSnapshotChanged(const QString &backendName,
                               const QString &snapshotJson,
                               const QString &reason,
                               const QString &windowId)
    {
        if (backendName != kKWinScriptBackendName) {
            return;
        }

        QString errorMessage;
        const std::optional<WindowSnapshot> snapshot = snapshotFromJsonText(snapshotJson, &errorMessage);
        if (!snapshot.has_value()) {
            emit m_owner->connectionFailed(errorMessage);
            return;
        }

        publishSnapshot(*snapshot, reason.isEmpty() ? QStringLiteral("window-updated") : reason, windowId);
    }

private:
    void publishSnapshot(const WindowSnapshot &snapshot, const QString &reason, const QString &windowId)
    {
        const bool wasReady = m_ready;
        m_snapshot = normalizeWindowSnapshot(snapshot);
        m_ready = true;

        if (!wasReady) {
            emit m_owner->initialSnapshotReady();
            return;
        }

        emit m_owner->windowStateChanged(reason, windowId);
    }

    KWinScriptWindowObserver *m_owner = nullptr;
    KWinScriptWindowBackendController m_controller;
    WindowSnapshot m_snapshot;
    bool m_started = false;
    bool m_ready = false;
};

KWinScriptWindowObserver::KWinScriptWindowObserver(QObject *parent)
    : WindowObserver(parent)
    , m_impl(std::make_unique<Impl>(this))
{
}

KWinScriptWindowObserver::~KWinScriptWindowObserver() = default;

void KWinScriptWindowObserver::start()
{
    m_impl->start();
}

const WindowSnapshot &KWinScriptWindowObserver::currentSnapshot() const
{
    return m_impl->currentSnapshot();
}

bool KWinScriptWindowObserver::hasInitialSnapshot() const
{
    return m_impl->hasInitialSnapshot();
}

QString KWinScriptWindowObserver::backendName() const
{
    return kKWinScriptBackendName;
}

} // namespace plasma_bridge::window

#include "kwin_script_window_backend.moc"
