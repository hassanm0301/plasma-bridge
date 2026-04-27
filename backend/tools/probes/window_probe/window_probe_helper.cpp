#include "common/window_state.h"
#include "plasma_bridge_build_config.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringList>
#include <QVariantMap>

#include <optional>

namespace
{

const QString kKWinServiceName = QStringLiteral("org.kde.KWin");
const QString kKWinObjectPath = QStringLiteral("/KWin");
const QString kKWinInterfaceName = QStringLiteral("org.kde.KWin");
const QString kCacheDirectoryName = QStringLiteral("plasma-bridge/window_probe");
const QString kSnapshotFileName = QStringLiteral("snapshot.json");

QString cacheRootPath()
{
    const QString override = qEnvironmentVariable("PLASMA_BRIDGE_WINDOW_PROBE_DATA_ROOT");
    const QString dataRoot =
        override.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) : override;
    return QDir(dataRoot).filePath(kCacheDirectoryName);
}

QString snapshotFilePath()
{
    return QDir(cacheRootPath()).filePath(kSnapshotFileName);
}

bool ensureDirectory(const QString &path)
{
    QDir dir;
    return dir.mkpath(path);
}

bool writeSnapshotFile(const QByteArray &jsonBytes)
{
    if (!ensureDirectory(cacheRootPath())) {
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

void applyBackfill(plasma_bridge::WindowState &window, const QVariantMap &windowInfo)
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

std::optional<plasma_bridge::WindowSnapshot> parseSnapshot(const QString &snapshotJson)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(snapshotJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return std::nullopt;
    }

    return plasma_bridge::windowSnapshotFromJson(document.object());
}

class WindowProbeHelperService final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", PLASMA_BRIDGE_WINDOW_PROBE_HELPER_INTERFACE)

public:
    explicit WindowProbeHelperService(QObject *parent = nullptr)
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
        const std::optional<plasma_bridge::WindowSnapshot> parsed = parseSnapshot(snapshotJson);
        if (!parsed.has_value()) {
            return false;
        }

        plasma_bridge::WindowSnapshot enriched = *parsed;
        for (plasma_bridge::WindowState &window : enriched.windows) {
            applyBackfill(window, windowInfoForId(window.id));
        }
        enriched = plasma_bridge::normalizeWindowSnapshot(enriched);

        m_backendName = backendName;
        m_snapshot = enriched;
        m_ready = true;
        m_snapshotJson =
            QString::fromUtf8(QJsonDocument(plasma_bridge::toJsonObject(m_snapshot)).toJson(QJsonDocument::Compact));

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

    void Shutdown()
    {
        QCoreApplication::quit();
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
        const std::optional<plasma_bridge::WindowSnapshot> snapshot = parseSnapshot(snapshotJson);
        if (!snapshot.has_value()) {
            return;
        }

        m_backendName = QStringLiteral("kwin-script-helper");
        m_snapshot = *snapshot;
        m_snapshotJson = snapshotJson;
        m_ready = true;
    }

    QString m_backendName = QStringLiteral("kwin-script-helper");
    plasma_bridge::WindowSnapshot m_snapshot;
    QString m_snapshotJson;
    QStringList m_activationRequests;
    bool m_ready = false;
};

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("window_probe_helper"));
    QCoreApplication::setApplicationVersion(QStringLiteral(PLASMA_BRIDGE_VERSION));

    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.registerService(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_SERVICE))) {
        return 1;
    }

    WindowProbeHelperService helper;
    if (!connection.registerObject(QStringLiteral(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_PATH),
                                   &helper,
                                   QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals)) {
        return 1;
    }

    return app.exec();
}

#include "window_probe_helper.moc"
