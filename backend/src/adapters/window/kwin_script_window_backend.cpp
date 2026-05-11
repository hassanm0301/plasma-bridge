#include "adapters/window/kwin_script_window_backend.h"

#include "plasma_bridge_build_config.h"

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
#include <QJsonParseError>
#include <QObject>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

namespace plasma_bridge::window
{
namespace
{

const QString kKWinScriptBackendName = QStringLiteral("kwin-script-helper");
const QString kKWinServiceName = QStringLiteral("org.kde.KWin");
const QString kKWinObjectPath = QStringLiteral("/KWin");
const QString kKWinInterfaceName = QStringLiteral("org.kde.KWin");
const QString kKWinScriptingObjectPath = QStringLiteral("/Scripting");
const QString kKWinScriptingInterfaceName = QStringLiteral("org.kde.kwin.Scripting");
const QString kCacheDirectoryName = QStringLiteral("plasma-bridge/window_probe");
const QString kSnapshotFileName = QStringLiteral("snapshot.json");
const QString kServiceDirectoryName = QStringLiteral("dbus-1/services");
const QString kServiceFileName = QStringLiteral("org.plasma_bridge.WindowProbe.service");
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
    if (typeof workspace.activateWindow === "function") {
        workspace.activateWindow(targetWindow);
    } else {
        workspace.activeWindow = targetWindow;
    }
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

bool writeSnapshotFile(const QByteArray &jsonBytes)
{
    QString errorMessage;
    if (!ensureDirectory(cacheRootPath(), &errorMessage)) {
        return false;
    }

    QSaveFile file(snapshotFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    if (file.write(jsonBytes) < 0) {
        return false;
    }

    return file.commit();
}

QString readString(const QVariantMap &map, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QVariant value = map.value(key);
        if (value.isValid() && !value.toString().isEmpty()) {
            return value.toString();
        }
    }

    return QString();
}

QString normalizedIconName(const QString &value)
{
    QString iconName = value.trimmed();
    if (iconName.isEmpty()) {
        return {};
    }

    if (iconName.endsWith(QStringLiteral(".desktop"))) {
        iconName.chop(QStringLiteral(".desktop").size());
    }

    if (iconName.contains(QLatin1Char('/'))) {
        const QFileInfo fileInfo(iconName);
        iconName = fileInfo.completeBaseName();
    }

    return iconName;
}

QString desktopFilePathForAppId(const QString &appId)
{
    const QString trimmed = appId.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    if (trimmed.contains(QLatin1Char('/'))) {
        const QFileInfo fileInfo(trimmed);
        return fileInfo.exists() && fileInfo.isFile() ? fileInfo.absoluteFilePath() : QString();
    }

    const QString desktopFileName = trimmed.endsWith(QStringLiteral(".desktop")) ? trimmed : trimmed + QStringLiteral(".desktop");
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                  QStringLiteral("applications/%1").arg(desktopFileName));
}

QString desktopIconNameForAppId(const QString &appId)
{
    const QString desktopFilePath = desktopFilePathForAppId(appId);
    if (desktopFilePath.isEmpty()) {
        return {};
    }

    QSettings settings(desktopFilePath, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Desktop Entry"));
    const QString iconName = settings.value(QStringLiteral("Icon")).toString();
    settings.endGroup();
    return iconName;
}

QString iconUrlForWindow(const WindowState &window, const QVariantMap &windowInfo)
{
    const QStringList candidates{
        desktopIconNameForAppId(window.appId),
        readString(windowInfo, {QStringLiteral("icon"), QStringLiteral("iconName")}),
        window.appId,
        window.resourceName,
        readString(windowInfo, {QStringLiteral("resourceClass"), QStringLiteral("desktopFileName")}),
    };

    for (const QString &candidate : candidates) {
        const QString iconName = normalizedIconName(candidate);
        if (!iconName.isEmpty()) {
            return QStringLiteral("/icons/apps/%1").arg(QString::fromUtf8(QUrl::toPercentEncoding(iconName)));
        }
    }

    return {};
}

std::optional<bool> readBool(const QVariantMap &map, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QVariant value = map.value(key);
        if (value.isValid()) {
            return value.toBool();
        }
    }

    return std::nullopt;
}

std::optional<quint32> readUInt(const QVariantMap &map, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QVariant value = map.value(key);
        if (value.isValid()) {
            bool ok = false;
            const quint32 converted = value.toUInt(&ok);
            if (ok) {
                return converted;
            }
        }
    }

    return std::nullopt;
}

QVariantMap windowInfoForId(const QString &windowId)
{
    QDBusInterface kwin(kKWinServiceName, kKWinObjectPath, kKWinInterfaceName, QDBusConnection::sessionBus());
    if (!kwin.isValid()) {
        return {};
    }

    const QDBusReply<QVariantMap> reply = kwin.call(QStringLiteral("getWindowInfo"), windowId);
    if (!reply.isValid()) {
        return {};
    }

    return reply.value();
}

void applyBackfill(WindowState &window, const QVariantMap &windowInfo)
{
    if (windowInfo.isEmpty()) {
        return;
    }

    if (window.title.isEmpty()) {
        window.title = readString(windowInfo, {QStringLiteral("caption"), QStringLiteral("title")});
    }

    if (window.appId.isEmpty()) {
        window.appId = readString(windowInfo,
                                  {QStringLiteral("desktopFileName"),
                                   QStringLiteral("resourceClass"),
                                   QStringLiteral("appId")});
    }

    if (window.resourceName.isEmpty()) {
        window.resourceName = readString(windowInfo, {QStringLiteral("resourceName")});
    }
    if (window.iconUrl.isEmpty()) {
        window.iconUrl = iconUrlForWindow(window, windowInfo);
    }

    if (window.pid == 0) {
        if (const std::optional<quint32> pid = readUInt(windowInfo, {QStringLiteral("pid")}); pid.has_value()) {
            window.pid = *pid;
        }
    }

    if (const std::optional<bool> active = readBool(windowInfo, {QStringLiteral("active")}); active.has_value()) {
        window.isActive = *active;
    }
    if (const std::optional<bool> minimized = readBool(windowInfo, {QStringLiteral("minimized")}); minimized.has_value()) {
        window.isMinimized = *minimized;
    }
    if (const std::optional<bool> fullscreen =
            readBool(windowInfo, {QStringLiteral("fullscreen"), QStringLiteral("fullScreen")});
        fullscreen.has_value()) {
        window.isFullscreen = *fullscreen;
    }
    if (const std::optional<bool> onAllDesktops = readBool(windowInfo, {QStringLiteral("onAllDesktops")});
        onAllDesktops.has_value()) {
        window.isOnAllDesktops = *onAllDesktops;
    }
    if (const std::optional<bool> skipTaskbar = readBool(windowInfo, {QStringLiteral("skipTaskbar")});
        skipTaskbar.has_value()) {
        window.skipTaskbar = *skipTaskbar;
    }
    if (const std::optional<bool> skipSwitcher = readBool(windowInfo, {QStringLiteral("skipSwitcher")});
        skipSwitcher.has_value()) {
        window.skipSwitcher = *skipSwitcher;
    }

    if (const std::optional<bool> maximized = readBool(windowInfo, {QStringLiteral("maximized")}); maximized.has_value()) {
        window.isMaximized = *maximized;
        return;
    }

    const std::optional<bool> horizontal = readBool(windowInfo,
                                                    {QStringLiteral("maximizeHorizontal"),
                                                     QStringLiteral("maximizedHorizontal"),
                                                     QStringLiteral("maximizedHorizontally")});
    const std::optional<bool> vertical = readBool(windowInfo,
                                                  {QStringLiteral("maximizeVertical"),
                                                   QStringLiteral("maximizedVertical"),
                                                   QStringLiteral("maximizedVertically")});
    if (horizontal.has_value() && vertical.has_value()) {
        window.isMaximized = *horizontal && *vertical;
    }
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

bool isHelperServiceRegistered()
{
    QDBusConnectionInterface *interface = QDBusConnection::sessionBus().interface();
    if (interface == nullptr) {
        return false;
    }

    const QDBusReply<bool> registered =
        interface->isServiceRegistered(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_SERVICE));
    return registered.isValid() && registered.value();
}

bool stopLegacyHelperServiceBestEffort()
{
    if (!isHelperServiceRegistered()) {
        return true;
    }

    QDBusInterface helper(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_SERVICE),
                          QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_PATH),
                          QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_INTERFACE),
                          QDBusConnection::sessionBus());
    helper.call(QStringLiteral("Shutdown"));
    return true;
}

bool unloadKWinScriptBestEffort()
{
    QDBusInterface scripting(kKWinServiceName,
                             kKWinScriptingObjectPath,
                             kKWinScriptingInterfaceName,
                             QDBusConnection::sessionBus());
    if (!scripting.isValid()) {
        return true;
    }

    scripting.call(QStringLiteral("unloadScript"), QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_SCRIPT_ID));
    return true;
}

bool startKWinScriptsBestEffort()
{
    QDBusInterface scripting(kKWinServiceName,
                             kKWinScriptingObjectPath,
                             kKWinScriptingInterfaceName,
                             QDBusConnection::sessionBus());
    if (!scripting.isValid()) {
        return true;
    }

    scripting.call(QStringLiteral("start"));
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

class WindowProbeDbusService final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", PLASMA_BRIDGE_WINDOW_PROBE_HELPER_INTERFACE)

public:
    explicit WindowProbeDbusService(QObject *parent = nullptr)
        : QObject(parent)
    {
        loadCachedSnapshot();
    }

public slots:
    bool PushSnapshot(const QString &backendName,
                      const QString &snapshotJson,
                      const QString &reason,
                      const QString &windowId)
    {
        const std::optional<WindowSnapshot> parsed = snapshotFromJsonText(snapshotJson);
        if (!parsed.has_value()) {
            return false;
        }

        WindowSnapshot enriched = *parsed;
        for (WindowState &window : enriched.windows) {
            applyBackfill(window, windowInfoForId(window.id));
        }
        enriched = normalizeWindowSnapshot(enriched);

        m_backendName = backendName;
        m_snapshot = enriched;
        m_ready = true;
        m_snapshotJson = QString::fromUtf8(QJsonDocument(toJsonObject(m_snapshot)).toJson(QJsonDocument::Compact));

        if (!writeSnapshotFile(m_snapshotJson.toUtf8())) {
            return false;
        }

        emit SnapshotChanged(m_backendName, m_snapshotJson, reason, windowId);
        return true;
    }

    QString GetSnapshot() const
    {
        return m_snapshotJson;
    }

    bool IsReady() const
    {
        return m_ready;
    }

    QString GetBackendName() const
    {
        return m_backendName;
    }

    bool RequestActivateWindow(const QString &windowId)
    {
        if (!m_ready || windowId.isEmpty()) {
            return false;
        }

        m_activationRequests.append(windowId);
        return true;
    }

    QString TakeActivationRequest()
    {
        if (m_activationRequests.isEmpty()) {
            return {};
        }

        return m_activationRequests.takeFirst();
    }

signals:
    void SnapshotChanged(const QString &backendName,
                         const QString &snapshotJson,
                         const QString &reason,
                         const QString &windowId);

private:
    void loadCachedSnapshot()
    {
        QFile file(snapshotFilePath());
        if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }

        const QString snapshotJson = QString::fromUtf8(file.readAll());
        const std::optional<WindowSnapshot> snapshot = snapshotFromJsonText(snapshotJson);
        if (!snapshot.has_value()) {
            return;
        }

        m_backendName = kKWinScriptBackendName;
        m_snapshot = *snapshot;
        m_snapshotJson = snapshotJson;
        m_ready = true;
    }

    QString m_backendName = kKWinScriptBackendName;
    WindowSnapshot m_snapshot;
    QString m_snapshotJson;
    QStringList m_activationRequests;
    bool m_ready = false;
};

WindowProbeDbusService *s_localHelperService = nullptr;

KWinScriptBackendStatus computeBackendStatus()
{
    KWinScriptBackendStatus status;
    status.backendName = kKWinScriptBackendName;
    status.scriptInstalled = QFileInfo::exists(kwinScriptMetadataFilePath()) && QFileInfo::exists(kwinScriptMainFilePath());
    status.scriptEnabled = isScriptEnabled();
    status.helperServiceInstalled = QFileInfo::exists(helperServiceFilePath());

    status.helperServiceRegistered = isHelperServiceRegistered();

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
    if (!status.scriptInstalled || !status.scriptEnabled) {
        return QStringLiteral("window_probe backend is not configured. Start `plasma_bridge` from a KDE Plasma session.");
    }

    if (!status.helperServiceRegistered) {
        return QStringLiteral("window_probe backend is not running. Start `plasma_bridge` first.");
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
    ~Impl()
    {
        unregisterHelperService();
    }

    KWinScriptBackendCommandResult setup()
    {
        KWinScriptBackendCommandResult result;
        result.status.backendName = kKWinScriptBackendName;

        QString errorMessage;
        stopLegacyHelperServiceBestEffort();
        if (!removePath(helperServiceFilePath(), &errorMessage)) {
            result.message = errorMessage;
            result.status = computeBackendStatus();
            return result;
        }

        if (!registerHelperService(&errorMessage)) {
            result.message = errorMessage;
            result.status = computeBackendStatus();
            return result;
        }

        if (!ensureDirectory(QDir(kwinScriptDirectoryPath()).filePath(kScriptCodeDirectoryName), &errorMessage)
            || !writeFileAtomically(kwinScriptMetadataFilePath(), scriptMetadataContents(), &errorMessage)
            || !writeFileAtomically(kwinScriptMainFilePath(), scriptMainContents(), &errorMessage)) {
            result.message = errorMessage;
            unregisterHelperService();
            result.status = computeBackendStatus();
            return result;
        }

        unloadKWinScriptBestEffort();
        setScriptEnabled(true);
        if (!reconfigureKWin(&errorMessage)) {
            result.message = errorMessage;
            unregisterHelperService();
            result.status = computeBackendStatus();
            return result;
        }
        startKWinScriptsBestEffort();

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
        unloadKWinScriptBestEffort();

        if (!removePath(kwinScriptDirectoryPath(), &errorMessage)
            || !removePath(helperServiceFilePath(), &errorMessage)
            || !removePath(snapshotFilePath(), &errorMessage)) {
            result.message = errorMessage;
            unregisterHelperService();
            result.status = computeBackendStatus();
            return result;
        }

        if (!reconfigureKWin(&errorMessage)) {
            result.message = errorMessage;
            unregisterHelperService();
            result.status = computeBackendStatus();
            return result;
        }

        unregisterHelperService();
        result.ok = true;
        result.message = QStringLiteral("Removed the KWin script backend for window state.");
        result.status = computeBackendStatus();
        return result;
    }

private:
    bool registerHelperService(QString *errorMessage)
    {
        if (m_serviceRegistered && m_objectRegistered) {
            return true;
        }

        m_helperService = std::make_unique<WindowProbeDbusService>();
        QDBusConnection connection = QDBusConnection::sessionBus();
        if (!connection.registerService(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_SERVICE))) {
            m_helperService.reset();
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Failed to register the Plasma Bridge window DBus service. "
                                               "Another plasma_bridge instance may already be running.");
            }
            return false;
        }
        m_serviceRegistered = true;

        if (!connection.registerObject(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_PATH),
                                       m_helperService.get(),
                                       QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals)) {
            connection.unregisterService(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_SERVICE));
            m_serviceRegistered = false;
            m_helperService.reset();
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Failed to register the Plasma Bridge window DBus object.");
            }
            return false;
        }
        m_objectRegistered = true;
        s_localHelperService = m_helperService.get();
        return true;
    }

    void unregisterHelperService()
    {
        QDBusConnection connection = QDBusConnection::sessionBus();
        if (m_objectRegistered) {
            connection.unregisterObject(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_PATH));
            m_objectRegistered = false;
        }
        if (m_serviceRegistered) {
            connection.unregisterService(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_SERVICE));
            m_serviceRegistered = false;
        }
        if (s_localHelperService == m_helperService.get()) {
            s_localHelperService = nullptr;
        }
        m_helperService.reset();
    }

    std::unique_ptr<WindowProbeDbusService> m_helperService;
    bool m_serviceRegistered = false;
    bool m_objectRegistered = false;
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

        if (s_localHelperService != nullptr) {
            if (!s_localHelperService->IsReady()) {
                result.status = control::WindowActivationStatus::NotReady;
                return result;
            }

            result.status = s_localHelperService->RequestActivateWindow(windowId)
                              ? control::WindowActivationStatus::Accepted
                              : control::WindowActivationStatus::WindowNotActivatable;
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

    ~Impl() override
    {
        if (m_started) {
            m_controller.teardown();
        }
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

        bool connected = false;
        if (s_localHelperService != nullptr) {
            connected = QObject::connect(s_localHelperService,
                                         SIGNAL(SnapshotChanged(QString, QString, QString, QString)),
                                         this,
                                         SLOT(handleSnapshotChanged(QString, QString, QString, QString)));
        } else {
            connected =
                QDBusConnection::sessionBus().connect(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_SERVICE),
                                                      QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_PATH),
                                                      QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_INTERFACE),
                                                      QStringLiteral("SnapshotChanged"),
                                                      this,
                                                      SLOT(handleSnapshotChanged(QString, QString, QString, QString)));
        }
        if (!connected) {
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

        if (s_localHelperService != nullptr) {
            const QString snapshotJson = s_localHelperService->GetSnapshot();
            if (snapshotJson.isEmpty()) {
                return;
            }

            QString errorMessage;
            const std::optional<WindowSnapshot> snapshot = snapshotFromJsonText(snapshotJson, &errorMessage);
            if (snapshot.has_value()) {
                publishSnapshot(*snapshot, QStringLiteral("initial"), QString());
            } else if (!errorMessage.isEmpty()) {
                emit m_owner->connectionFailed(errorMessage);
            }
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
