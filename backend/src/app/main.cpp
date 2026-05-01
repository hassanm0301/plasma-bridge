#include "plasma_bridge_build_config.h"
#include "adapters/audio/pulse_audio_device_controller.h"
#include "adapters/audio/pulse_audio_state_observer.h"
#include "adapters/audio/pulse_audio_volume_controller.h"
#include "adapters/media/mpris_media_controller.h"
#include "adapters/media/mpris_media_observer.h"
#include "adapters/window/kwin_script_window_backend.h"
#include "api/snapshot_http_server.h"
#include "api/state_websocket_server.h"
#include "state/audio_state_store.h"
#include "state/media_state_store.h"
#include "state/window_state_store.h"

#include <PulseAudioQt/Context>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QHostAddress>
#include <QTextStream>

namespace
{

bool parseHostAddress(const QString &value, QHostAddress *outAddress)
{
    if (outAddress == nullptr) {
        return false;
    }

    if (value == QStringLiteral("localhost")) {
        *outAddress = QHostAddress(QHostAddress::LocalHost);
        return true;
    }

    QHostAddress address;
    if (!address.setAddress(value)) {
        return false;
    }

    *outAddress = address;
    return true;
}

QString urlHost(const QString &host)
{
    return host.contains(QLatin1Char(':')) && !host.startsWith(QLatin1Char('[')) ? QStringLiteral("[%1]").arg(host) : host;
}

bool parseAllowedOrigins(const QStringList &values,
                         QList<plasma_bridge::api::AllowedOrigin> *outOrigins,
                         QString *errorValue,
                         QString *errorMessage)
{
    if (outOrigins == nullptr) {
        return false;
    }

    outOrigins->clear();
    for (const QString &value : values) {
        plasma_bridge::api::AllowedOrigin origin;
        QString parseError;
        if (!plasma_bridge::api::SnapshotHttpServer::parseAllowedOrigin(value, &origin, &parseError)) {
            if (errorValue != nullptr) {
                *errorValue = value;
            }
            if (errorMessage != nullptr) {
                *errorMessage = parseError;
            }
            return false;
        }
        outOrigins->append(origin);
    }

    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(docs_resources);

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("plasma_bridge"));
    QCoreApplication::setApplicationVersion(QStringLiteral(PLASMA_BRIDGE_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Serve audio, media, and window state over HTTP and WebSocket."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption hostOption(QStringLiteral("host"),
                                  QStringLiteral("Host address to bind. Defaults to %1.")
                                      .arg(QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST)),
                                  QStringLiteral("host"),
                                  QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST));
    QCommandLineOption portOption(QStringLiteral("port"),
                                  QStringLiteral("TCP port to bind. Defaults to %1.")
                                      .arg(QString::number(PLASMA_BRIDGE_DEFAULT_HTTP_PORT)),
                                  QStringLiteral("port"),
                                  QString::number(PLASMA_BRIDGE_DEFAULT_HTTP_PORT));
    QCommandLineOption wsPortOption(QStringLiteral("ws-port"),
                                    QStringLiteral("WebSocket port to bind. Defaults to %1.")
                                        .arg(QString::number(PLASMA_BRIDGE_DEFAULT_WS_PORT)),
                                    QStringLiteral("ws-port"),
                                    QString::number(PLASMA_BRIDGE_DEFAULT_WS_PORT));
    QCommandLineOption allowOriginOption(QStringLiteral("allow-origin"),
                                         QStringLiteral("Additional browser origin allowed by CORS. Repeat to allow multiple origins."),
                                         QStringLiteral("origin"));

    parser.addOption(hostOption);
    parser.addOption(portOption);
    parser.addOption(wsPortOption);
    parser.addOption(allowOriginOption);
    parser.process(app);

    QHostAddress bindAddress;
    if (!parseHostAddress(parser.value(hostOption), &bindAddress)) {
        QTextStream error(stderr);
        error << "Invalid host address: " << parser.value(hostOption) << Qt::endl;
        return 1;
    }

    bool portOk = false;
    const quint16 port = parser.value(portOption).toUShort(&portOk);
    if (!portOk || port == 0) {
        QTextStream error(stderr);
        error << "Invalid port: " << parser.value(portOption) << Qt::endl;
        return 1;
    }

    bool wsPortOk = false;
    const quint16 wsPort = parser.value(wsPortOption).toUShort(&wsPortOk);
    if (!wsPortOk || wsPort == 0) {
        QTextStream error(stderr);
        error << "Invalid WebSocket port: " << parser.value(wsPortOption) << Qt::endl;
        return 1;
    }

    QList<plasma_bridge::api::AllowedOrigin> allowedOrigins;
    QString invalidOriginValue;
    QString invalidOriginMessage;
    if (!parseAllowedOrigins(parser.values(allowOriginOption), &allowedOrigins, &invalidOriginValue, &invalidOriginMessage)) {
        QTextStream error(stderr);
        error << "Invalid allowed origin: " << invalidOriginValue << ". " << invalidOriginMessage << Qt::endl;
        return 1;
    }

    PulseAudioQt::Context::setApplicationId(QStringLiteral(PLASMA_BRIDGE_APP_ID));

    plasma_bridge::audio::PulseAudioStateObserver observer;
    plasma_bridge::audio::PulseAudioVolumeController volumeController;
    plasma_bridge::audio::PulseAudioDeviceController deviceController;
    plasma_bridge::state::AudioStateStore audioStateStore;
    audioStateStore.attachObserver(&observer);
    plasma_bridge::media::MprisMediaObserver mediaObserver;
    plasma_bridge::state::MediaStateStore mediaStateStore;
    mediaStateStore.attachObserver(&mediaObserver);
    plasma_bridge::media::MprisMediaController mediaController(&mediaStateStore);
    plasma_bridge::window::KWinScriptWindowObserver windowObserver;
    plasma_bridge::state::WindowStateStore windowStateStore;
    windowStateStore.attachObserver(&windowObserver);
    plasma_bridge::window::KWinScriptWindowActivationController windowActivationController;

    plasma_bridge::api::SnapshotHttpServer httpServer(&audioStateStore,
                                                      &mediaStateStore,
                                                      &windowStateStore,
                                                      &volumeController,
                                                      &deviceController,
                                                      &mediaController,
                                                      &windowActivationController,
                                                      parser.value(hostOption),
                                                      port,
                                                      wsPort,
                                                      allowedOrigins);
    if (!httpServer.listen(bindAddress, port)) {
        QTextStream error(stderr);
        error << "Failed to listen on " << parser.value(hostOption) << ':' << port << ": "
              << httpServer.errorString() << Qt::endl;
        return 1;
    }

    plasma_bridge::api::StateWebSocketServer webSocketServer(&audioStateStore, &mediaStateStore, &windowStateStore);
    if (!webSocketServer.listen(bindAddress, wsPort)) {
        QTextStream error(stderr);
        error << "Failed to listen on " << parser.value(hostOption) << ':' << wsPort << " for WebSocket: "
              << webSocketServer.errorString() << Qt::endl;
        return 1;
    }

    QObject::connect(&observer, &plasma_bridge::audio::PulseAudioStateObserver::connectionFailed, &app, [](const QString &message) {
        QTextStream error(stderr);
        error << message << Qt::endl;
    });
    QObject::connect(&mediaObserver, &plasma_bridge::media::MprisMediaObserver::connectionFailed, &app, [](const QString &message) {
        QTextStream error(stderr);
        error << message << Qt::endl;
    });
    QObject::connect(&windowObserver, &plasma_bridge::window::KWinScriptWindowObserver::connectionFailed, &app, [](const QString &message) {
        QTextStream error(stderr);
        error << message << Qt::endl;
    });

    observer.start();
    mediaObserver.start();
    windowObserver.start();

    QTextStream output(stdout);
    const QString formattedHost = urlHost(parser.value(hostOption));
    const QString httpBaseUrl = QStringLiteral("http://%1:%2").arg(formattedHost, QString::number(httpServer.serverPort()));
    const QString wsEndpointUrl =
        QStringLiteral("ws://%1:%2%3").arg(formattedHost,
                                           QString::number(webSocketServer.serverPort()),
                                           plasma_bridge::api::StateWebSocketServer::endpointPath());
    const QString docsIndexUrl = httpBaseUrl + QStringLiteral("/docs/");
    const QString httpDocsUrl = httpBaseUrl + QStringLiteral("/docs/http");
    const QString wsDocsUrl = httpBaseUrl + QStringLiteral("/docs/ws");

    output << "Listening on " << httpBaseUrl << Qt::endl;
    output << "Listening on " << wsEndpointUrl << Qt::endl;
    output << "Docs index: " << docsIndexUrl << Qt::endl;
    output << "HTTP docs: " << httpDocsUrl << Qt::endl;
    output << "WebSocket docs: " << wsDocsUrl << Qt::endl;

    return app.exec();
}
