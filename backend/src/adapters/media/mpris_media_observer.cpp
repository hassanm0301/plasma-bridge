#include "adapters/media/mpris_media_observer.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusVariant>
#include <QSet>
#include <QTimer>
#include <QUrl>

#include <chrono>

namespace plasma_bridge::media
{
namespace
{

const QString kDbusServiceName = QStringLiteral("org.freedesktop.DBus");
const QString kDbusPath = QStringLiteral("/org/freedesktop/DBus");
const QString kDbusInterface = QStringLiteral("org.freedesktop.DBus");
const QString kDbusPropertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");
const QString kMprisServicePrefix = QStringLiteral("org.mpris.MediaPlayer2.");
const QString kMprisObjectPath = QStringLiteral("/org/mpris/MediaPlayer2");
const QString kMprisRootInterface = QStringLiteral("org.mpris.MediaPlayer2");
const QString kMprisPlayerInterface = QStringLiteral("org.mpris.MediaPlayer2.Player");
const QString kAppIconPathPrefix = QStringLiteral("/icons/apps/");
constexpr auto kPositionPollInterval = std::chrono::milliseconds(1000);

QVariantMap variantMapFromVariant(const QVariant &value)
{
    if (value.canConvert<QVariantMap>()) {
        return value.toMap();
    }

    const QDBusArgument argument = qvariant_cast<QDBusArgument>(value);
    QVariantMap result;
    if (argument.currentType() != QDBusArgument::MapType) {
        return result;
    }

    argument.beginMap();
    while (!argument.atEnd()) {
        QString key;
        QDBusVariant dbusValue;
        argument.beginMapEntry();
        argument >> key >> dbusValue;
        argument.endMapEntry();
        result.insert(key, dbusValue.variant());
    }
    argument.endMap();
    return result;
}

QStringList stringListFromVariant(const QVariant &value)
{
    if (value.canConvert<QStringList>()) {
        return value.toStringList();
    }

    if (value.typeId() == QMetaType::QString) {
        return {value.toString()};
    }

    const QDBusArgument argument = qvariant_cast<QDBusArgument>(value);
    QStringList result;
    if (argument.currentType() != QDBusArgument::ArrayType) {
        return result;
    }

    argument.beginArray();
    while (!argument.atEnd()) {
        QString item;
        argument >> item;
        result.append(item);
    }
    argument.endArray();
    return result;
}

QString stringFromVariant(const QVariant &value)
{
    return value.canConvert<QString>() ? value.toString().trimmed() : QString();
}

std::optional<qint64> trackLengthMsFromVariant(const QVariant &value)
{
    bool ok = false;
    const qlonglong microseconds = value.toLongLong(&ok);
    if (!ok || microseconds < 0) {
        return std::nullopt;
    }

    return microseconds / 1000;
}

std::optional<qint64> positionMsFromVariant(const QVariant &value)
{
    return trackLengthMsFromVariant(value);
}

QString trackIdFromMetadata(const QVariantMap &metadata)
{
    const QVariant trackIdValue = metadata.value(QStringLiteral("mpris:trackid"));
    if (trackIdValue.canConvert<QDBusObjectPath>()) {
        return trackIdValue.value<QDBusObjectPath>().path();
    }

    if (trackIdValue.typeId() == QMetaType::QString) {
        return trackIdValue.toString().trimmed();
    }

    return {};
}

plasma_bridge::MediaPlaybackStatus playbackStatusFromString(const QString &value)
{
    if (value.compare(QStringLiteral("Playing"), Qt::CaseInsensitive) == 0) {
        return plasma_bridge::MediaPlaybackStatus::Playing;
    }
    if (value.compare(QStringLiteral("Paused"), Qt::CaseInsensitive) == 0) {
        return plasma_bridge::MediaPlaybackStatus::Paused;
    }
    if (value.compare(QStringLiteral("Stopped"), Qt::CaseInsensitive) == 0) {
        return plasma_bridge::MediaPlaybackStatus::Stopped;
    }
    return plasma_bridge::MediaPlaybackStatus::Unknown;
}

QString iconUrlForDesktopEntry(const QString &desktopEntry)
{
    const QString trimmed = desktopEntry.trimmed();
    if (trimmed.isEmpty() || trimmed.contains(QLatin1Char('/'))) {
        return {};
    }

    return QStringLiteral("%1%2")
        .arg(kAppIconPathPrefix, QString::fromUtf8(QUrl::toPercentEncoding(trimmed)));
}

QString artworkUrlFromMetadata(const QVariantMap &metadata)
{
    const QUrl url(metadata.value(QStringLiteral("mpris:artUrl")).toString());
    if (!url.isValid()) {
        return {};
    }

    const QString scheme = url.scheme().toLower();
    if (scheme != QStringLiteral("http") && scheme != QStringLiteral("https")) {
        return {};
    }

    return url.toString(QUrl::FullyEncoded);
}

plasma_bridge::MediaPlayerState playerStateFromProperties(const QString &service,
                                                          const QVariantMap &rootProperties,
                                                          const QVariantMap &playerProperties,
                                                          const quint64 updateSequence)
{
    plasma_bridge::MediaPlayerState state;
    state.playerId = service;
    state.identity = stringFromVariant(rootProperties.value(QStringLiteral("Identity")));
    state.desktopEntry = stringFromVariant(rootProperties.value(QStringLiteral("DesktopEntry")));
    state.playbackStatus = playbackStatusFromString(playerProperties.value(QStringLiteral("PlaybackStatus")).toString());
    state.canPlay = playerProperties.value(QStringLiteral("CanPlay")).toBool();
    state.canPause = playerProperties.value(QStringLiteral("CanPause")).toBool();
    state.canGoNext = playerProperties.value(QStringLiteral("CanGoNext")).toBool();
    state.canGoPrevious = playerProperties.value(QStringLiteral("CanGoPrevious")).toBool();
    state.canControl = playerProperties.value(QStringLiteral("CanControl")).toBool();
    state.canSeek = playerProperties.value(QStringLiteral("CanSeek")).toBool();
    state.positionMs = positionMsFromVariant(playerProperties.value(QStringLiteral("Position")));
    state.updateSequence = updateSequence;

    const QVariantMap metadata = variantMapFromVariant(playerProperties.value(QStringLiteral("Metadata")));
    state.title = stringFromVariant(metadata.value(QStringLiteral("xesam:title")));
    state.artists = stringListFromVariant(metadata.value(QStringLiteral("xesam:artist")));
    state.album = stringFromVariant(metadata.value(QStringLiteral("xesam:album")));
    state.trackLengthMs = trackLengthMsFromVariant(metadata.value(QStringLiteral("mpris:length")));
    state.trackId = trackIdFromMetadata(metadata);
    state.artworkUrl = artworkUrlFromMetadata(metadata);
    state.appIconUrl = iconUrlForDesktopEntry(state.desktopEntry);
    return state;
}

} // namespace

class MprisMediaObserver::Impl final : public QObject
{
    Q_OBJECT

public:
    explicit Impl(MprisMediaObserver *owner)
        : QObject(owner)
        , m_owner(owner)
    {
        m_positionPollTimer.setInterval(kPositionPollInterval);
        connect(&m_positionPollTimer, &QTimer::timeout, this, &Impl::handlePositionPollTimeout);
    }

    void start()
    {
        if (m_started) {
            return;
        }

        m_started = true;
        if (!m_bus.isConnected()) {
            emit m_owner->connectionFailed(QStringLiteral("Failed to connect to the session DBus bus."));
            return;
        }

        if (!m_bus.connect(kDbusServiceName,
                           kDbusPath,
                           kDbusInterface,
                           QStringLiteral("NameOwnerChanged"),
                           this,
                           SLOT(handleNameOwnerChanged(QString,QString,QString)))) {
            emit m_owner->connectionFailed(QStringLiteral("Failed to subscribe to DBus name-owner changes."));
            return;
        }

        refreshAllPlayers(QStringLiteral("initial"), QString());
    }

    const plasma_bridge::MediaState &currentState() const
    {
        return m_currentState;
    }

    bool hasInitialState() const
    {
        return m_hasInitialState;
    }

public slots:
    void handleNameOwnerChanged(const QString &service, const QString &oldOwner, const QString &newOwner)
    {
        if (!service.startsWith(kMprisServicePrefix)) {
            return;
        }

        if (oldOwner.isEmpty() && !newOwner.isEmpty()) {
            refreshSinglePlayer(service, QStringLiteral("player-added"));
            return;
        }

        if (!oldOwner.isEmpty() && newOwner.isEmpty()) {
            disconnectPlayerSignals(service);
            m_players.remove(service);
            publishState(QStringLiteral("player-removed"), service);
            return;
        }

        refreshSinglePlayer(service, QStringLiteral("player-replaced"));
    }

    void handlePropertiesChanged(const QString &interfaceName,
                                 const QVariantMap &changedProperties,
                                 const QStringList &invalidatedProperties,
                                 const QDBusMessage &message)
    {
        Q_UNUSED(changedProperties)
        Q_UNUSED(invalidatedProperties)

        if (interfaceName != kMprisRootInterface && interfaceName != kMprisPlayerInterface) {
            return;
        }

        const QString service = message.service();
        if (service.isEmpty()) {
            return;
        }

        refreshSinglePlayer(service, QStringLiteral("player-updated"));
    }

    void handlePositionPollTimeout()
    {
        if (!m_currentState.player.has_value()) {
            m_positionPollTimer.stop();
            return;
        }

        const QString service = m_currentState.player->playerId;
        const std::optional<plasma_bridge::MediaPlayerState> player = fetchPlayerState(service);
        if (!player.has_value()) {
            disconnectPlayerSignals(service);
            m_players.remove(service);
            publishState(QStringLiteral("player-removed"), service);
            return;
        }

        m_players.insert(service, *player);
        publishState(QStringLiteral("position-updated"), service);
    }

private:
    void refreshAllPlayers(const QString &reason, const QString &playerId)
    {
        QDBusConnectionInterface *connectionInterface = m_bus.interface();
        if (connectionInterface == nullptr) {
            emit m_owner->connectionFailed(QStringLiteral("Failed to query session DBus services."));
            return;
        }

        const QDBusReply<QStringList> namesReply = connectionInterface->registeredServiceNames();
        if (!namesReply.isValid()) {
            emit m_owner->connectionFailed(QStringLiteral("Failed to query session DBus services."));
            return;
        }

        QSet<QString> visiblePlayers;
        for (const QString &service : namesReply.value()) {
            if (!service.startsWith(kMprisServicePrefix)) {
                continue;
            }
            visiblePlayers.insert(service);
            upsertPlayer(service);
        }

        const QStringList knownServices = m_players.keys();
        for (const QString &service : knownServices) {
            if (!visiblePlayers.contains(service)) {
                disconnectPlayerSignals(service);
                m_players.remove(service);
            }
        }

        publishState(reason, playerId);
        if (!m_hasInitialState) {
            m_hasInitialState = true;
            emit m_owner->initialStateReady();
        }
    }

    void refreshSinglePlayer(const QString &service, const QString &reason)
    {
        upsertPlayer(service);
        publishState(reason, service);
        if (!m_hasInitialState) {
            m_hasInitialState = true;
            emit m_owner->initialStateReady();
        }
    }

    void upsertPlayer(const QString &service)
    {
        const std::optional<plasma_bridge::MediaPlayerState> player = fetchPlayerState(service);
        if (!player.has_value()) {
            disconnectPlayerSignals(service);
            m_players.remove(service);
            return;
        }

        connectPlayerSignals(service);
        m_players.insert(service, *player);
    }

    std::optional<plasma_bridge::MediaPlayerState> fetchPlayerState(const QString &service)
    {
        QDBusInterface propertiesInterface(service, kMprisObjectPath, kDbusPropertiesInterface, m_bus);
        if (!propertiesInterface.isValid()) {
            return std::nullopt;
        }

        const QDBusReply<QVariantMap> rootReply = propertiesInterface.call(QStringLiteral("GetAll"), kMprisRootInterface);
        const QDBusReply<QVariantMap> playerReply = propertiesInterface.call(QStringLiteral("GetAll"), kMprisPlayerInterface);
        if (!rootReply.isValid() || !playerReply.isValid()) {
            return std::nullopt;
        }

        ++m_updateSequence;
        return playerStateFromProperties(service, rootReply.value(), playerReply.value(), m_updateSequence);
    }

    void connectPlayerSignals(const QString &service)
    {
        if (m_connectedServices.contains(service)) {
            return;
        }

        if (m_bus.connect(service,
                          kMprisObjectPath,
                          kDbusPropertiesInterface,
                          QStringLiteral("PropertiesChanged"),
                          this,
                          SLOT(handlePropertiesChanged(QString,QVariantMap,QStringList,QDBusMessage)))) {
            m_connectedServices.insert(service);
        }
    }

    void disconnectPlayerSignals(const QString &service)
    {
        if (!m_connectedServices.remove(service)) {
            return;
        }

        m_bus.disconnect(service,
                         kMprisObjectPath,
                         kDbusPropertiesInterface,
                         QStringLiteral("PropertiesChanged"),
                         this,
                         SLOT(handlePropertiesChanged(QString,QVariantMap,QStringList,QDBusMessage)));
    }

    void publishState(const QString &reason, const QString &playerId)
    {
        const std::optional<plasma_bridge::MediaPlayerState> currentPlayer =
            plasma_bridge::selectCurrentMediaPlayer(m_players.values());
        m_currentState.player = currentPlayer;
        updatePositionPolling();
        emit m_owner->mediaStateChanged(reason, playerId);
    }

    void updatePositionPolling()
    {
        if (!m_currentState.player.has_value() || m_currentState.player->playerId.isEmpty()
            || m_currentState.player->playbackStatus != plasma_bridge::MediaPlaybackStatus::Playing) {
            m_positionPollTimer.stop();
            return;
        }

        if (!m_positionPollTimer.isActive()) {
            m_positionPollTimer.start();
        }
    }

    MprisMediaObserver *m_owner = nullptr;
    QDBusConnection m_bus = QDBusConnection::sessionBus();
    QTimer m_positionPollTimer;
    QHash<QString, plasma_bridge::MediaPlayerState> m_players;
    QSet<QString> m_connectedServices;
    plasma_bridge::MediaState m_currentState;
    quint64 m_updateSequence = 0;
    bool m_started = false;
    bool m_hasInitialState = false;
};

MprisMediaObserver::MprisMediaObserver(QObject *parent)
    : MediaObserver(parent)
    , m_impl(std::make_unique<Impl>(this))
{
}

MprisMediaObserver::~MprisMediaObserver() = default;

void MprisMediaObserver::start()
{
    m_impl->start();
}

const plasma_bridge::MediaState &MprisMediaObserver::currentState() const
{
    return m_impl->currentState();
}

bool MprisMediaObserver::hasInitialState() const
{
    return m_impl->hasInitialState();
}

} // namespace plasma_bridge::media

#include "mpris_media_observer.moc"
