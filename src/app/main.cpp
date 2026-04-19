#include "plasma_bridge_build_config.h"
#include "adapters/audio/pulse_audio_device_controller.h"
#include "adapters/audio/pulse_audio_state_observer.h"
#include "adapters/audio/pulse_audio_volume_controller.h"
#include "api/audio_websocket_server.h"
#include "api/snapshot_http_server.h"
#include "state/audio_state_store.h"

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

} // namespace

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(docs_resources);

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("plasma_bridge"));
    QCoreApplication::setApplicationVersion(QStringLiteral(PLASMA_BRIDGE_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Serve audio sink and source state over HTTP and WebSocket."));
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

    parser.addOption(hostOption);
    parser.addOption(portOption);
    parser.addOption(wsPortOption);
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

    PulseAudioQt::Context::setApplicationId(QStringLiteral(PLASMA_BRIDGE_APP_ID));

    plasma_bridge::audio::PulseAudioStateObserver observer;
    plasma_bridge::audio::PulseAudioVolumeController volumeController;
    plasma_bridge::audio::PulseAudioDeviceController deviceController;
    plasma_bridge::state::AudioStateStore audioStateStore;
    audioStateStore.attachObserver(&observer);

    plasma_bridge::api::SnapshotHttpServer httpServer(&audioStateStore,
                                                      &volumeController,
                                                      &deviceController,
                                                      parser.value(hostOption),
                                                      port,
                                                      wsPort);
    if (!httpServer.listen(bindAddress, port)) {
        QTextStream error(stderr);
        error << "Failed to listen on " << parser.value(hostOption) << ':' << port << ": "
              << httpServer.errorString() << Qt::endl;
        return 1;
    }

    plasma_bridge::api::AudioWebSocketServer webSocketServer(&audioStateStore);
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

    observer.start();

    QTextStream output(stdout);
    const QString formattedHost = urlHost(parser.value(hostOption));
    const QString httpBaseUrl = QStringLiteral("http://%1:%2").arg(formattedHost, QString::number(httpServer.serverPort()));
    const QString wsEndpointUrl =
        QStringLiteral("ws://%1:%2%3").arg(formattedHost,
                                           QString::number(webSocketServer.serverPort()),
                                           plasma_bridge::api::AudioWebSocketServer::endpointPath());
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
