#include "adapters/audio/pulse_audio_sink_observer.h"
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

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("plasma_bridge"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Serve audio sink state over HTTP and WebSocket."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption hostOption(QStringLiteral("host"),
                                  QStringLiteral("Host address to bind. Defaults to 127.0.0.1."),
                                  QStringLiteral("host"),
                                  QStringLiteral("127.0.0.1"));
    QCommandLineOption portOption(QStringLiteral("port"),
                                  QStringLiteral("TCP port to bind. Defaults to 8080."),
                                  QStringLiteral("port"),
                                  QStringLiteral("8080"));
    QCommandLineOption wsPortOption(QStringLiteral("ws-port"),
                                    QStringLiteral("WebSocket port to bind. Defaults to 8081."),
                                    QStringLiteral("ws-port"),
                                    QStringLiteral("8081"));

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

    PulseAudioQt::Context::setApplicationId(QStringLiteral("org.plasma-remote-toolbar.plasma_bridge"));

    plasma_bridge::audio::PulseAudioSinkObserver observer;
    plasma_bridge::state::AudioStateStore audioStateStore;
    audioStateStore.attachObserver(&observer);

    plasma_bridge::api::SnapshotHttpServer httpServer(&audioStateStore);
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

    QObject::connect(&observer, &plasma_bridge::audio::PulseAudioSinkObserver::connectionFailed, &app, [](const QString &message) {
        QTextStream error(stderr);
        error << message << Qt::endl;
    });

    observer.start();

    QTextStream output(stdout);
    output << "Listening on http://" << parser.value(hostOption) << ':' << httpServer.serverPort() << Qt::endl;
    output << "Listening on ws://" << parser.value(hostOption) << ':' << webSocketServer.serverPort() << Qt::endl;

    return app.exec();
}
