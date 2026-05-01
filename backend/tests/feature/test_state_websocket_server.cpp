#include "api/state_websocket_server.h"
#include "state/audio_state_store.h"
#include "state/media_state_store.h"
#include "state/window_state_store.h"
#include "tests/support/test_support.h"

#include "plasma_bridge_build_config.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QWebSocket>
#include <QtTest>

class StateWebSocketServerFeatureTest : public QObject
{
    Q_OBJECT

private slots:
    void rejectsUnknownEndpoint();
    void ignoresStateChangesUntilHello();
    void rejectsMalformedFirstMessages();
    void rejectsUnsupportedProtocolVersionAndBinaryMessages();
    void sendsNotReadyThenFullStateAfterReadiness();
    void sendsPatchMessagesToAllReadyClients();
    void sendsMediaNotReadyThenFullStateAfterReadiness();
    void sendsMediaPatchMessagesToAllReadyClients();
    void sendsWindowNotReadyThenFullStateAfterReadiness();
    void sendsWindowPatchMessagesToAllReadyClients();

private:
    static QHostAddress bindAddress();
    static QString helloMessage(int protocolVersion = plasma_bridge::api::StateWebSocketServer::protocolVersion());
    static QJsonObject takeMessage(QSignalSpy &spy, int index = 0);
};

namespace
{

QJsonObject payloadObject(const QJsonObject &envelope)
{
    return envelope.value(QStringLiteral("payload")).toObject();
}

QJsonObject errorObject(const QJsonObject &envelope)
{
    return envelope.value(QStringLiteral("error")).toObject();
}

} // namespace

void StateWebSocketServerFeatureTest::rejectsUnknownEndpoint()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::api::StateWebSocketServer server(&store);
    QVERIFY(server.listen(bindAddress(), 0));

    QWebSocket socket;
    QSignalSpy connectedSpy(&socket, &QWebSocket::connected);
    QSignalSpy messageSpy(&socket, &QWebSocket::textMessageReceived);
    QSignalSpy disconnectedSpy(&socket, &QWebSocket::disconnected);

    socket.open(plasma_bridge::tests::wsUrl(server.serverPort(), QStringLiteral("/ws/unknown")));

    QTRY_COMPARE(connectedSpy.count(), 1);
    QTRY_COMPARE(messageSpy.count(), 1);
    QTRY_COMPARE(disconnectedSpy.count(), 1);

    const QJsonObject error = takeMessage(messageSpy);
    QCOMPARE(error.value(QStringLiteral("type")).toString(), QStringLiteral("error"));
    QVERIFY(error.value(QStringLiteral("payload")).isNull());
    QCOMPARE(errorObject(error).value(QStringLiteral("code")).toString(), QStringLiteral("unknown_endpoint"));
}

void StateWebSocketServerFeatureTest::ignoresStateChangesUntilHello()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::api::StateWebSocketServer server(&store);
    QVERIFY(server.listen(bindAddress(), 0));

    QWebSocket socket;
    QSignalSpy connectedSpy(&socket, &QWebSocket::connected);
    QSignalSpy messageSpy(&socket, &QWebSocket::textMessageReceived);
    QSignalSpy disconnectedSpy(&socket, &QWebSocket::disconnected);

    socket.open(plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::StateWebSocketServer::endpointPath()));
    QTRY_COMPARE(connectedSpy.count(), 1);

    store.updateAudioState(plasma_bridge::tests::alternateAudioState(), true, QStringLiteral("sink-updated"), QStringLiteral("bluez_output.headset.1"));
    QTest::qWait(50);
    QCOMPARE(messageSpy.count(), 0);

    socket.sendTextMessage(helloMessage());
    QTRY_COMPARE(messageSpy.count(), 1);
    const QJsonObject fullState = takeMessage(messageSpy);
    QCOMPARE(fullState.value(QStringLiteral("type")).toString(), QStringLiteral("fullState"));
    QVERIFY(fullState.value(QStringLiteral("error")).isNull());
    const QJsonObject audio = payloadObject(fullState).value(QStringLiteral("audio")).toObject();
    QCOMPARE(audio.value(QStringLiteral("sources")).toArray().size(), store.audioState().sources.size());
    QCOMPARE(audio.value(QStringLiteral("selectedSourceId")).toString(), store.audioState().selectedSourceId);

    socket.close();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
}

void StateWebSocketServerFeatureTest::rejectsMalformedFirstMessages()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::api::StateWebSocketServer server(&store);
    QVERIFY(server.listen(bindAddress(), 0));

    QWebSocket malformedSocket;
    QSignalSpy malformedConnectedSpy(&malformedSocket, &QWebSocket::connected);
    QSignalSpy malformedMessageSpy(&malformedSocket, &QWebSocket::textMessageReceived);
    QSignalSpy malformedDisconnectedSpy(&malformedSocket, &QWebSocket::disconnected);
    malformedSocket.open(plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::StateWebSocketServer::endpointPath()));
    QTRY_COMPARE(malformedConnectedSpy.count(), 1);
    malformedSocket.sendTextMessage(QStringLiteral("{"));
    QTRY_COMPARE(malformedMessageSpy.count(), 1);
    QTRY_COMPARE(malformedDisconnectedSpy.count(), 1);
    QCOMPARE(errorObject(takeMessage(malformedMessageSpy)).value(QStringLiteral("code")).toString(), QStringLiteral("bad_request"));

    QWebSocket wrongTypeSocket;
    QSignalSpy wrongTypeConnectedSpy(&wrongTypeSocket, &QWebSocket::connected);
    QSignalSpy wrongTypeMessageSpy(&wrongTypeSocket, &QWebSocket::textMessageReceived);
    QSignalSpy wrongTypeDisconnectedSpy(&wrongTypeSocket, &QWebSocket::disconnected);
    wrongTypeSocket.open(plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::StateWebSocketServer::endpointPath()));
    QTRY_COMPARE(wrongTypeConnectedSpy.count(), 1);
    wrongTypeSocket.sendTextMessage(QStringLiteral("{\"type\":\"patch\"}"));
    QTRY_COMPARE(wrongTypeMessageSpy.count(), 1);
    QTRY_COMPARE(wrongTypeDisconnectedSpy.count(), 1);
    QCOMPARE(errorObject(takeMessage(wrongTypeMessageSpy)).value(QStringLiteral("code")).toString(), QStringLiteral("bad_request"));
}

void StateWebSocketServerFeatureTest::rejectsUnsupportedProtocolVersionAndBinaryMessages()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::api::StateWebSocketServer server(&store);
    QVERIFY(server.listen(bindAddress(), 0));

    QWebSocket versionSocket;
    QSignalSpy versionConnectedSpy(&versionSocket, &QWebSocket::connected);
    QSignalSpy versionMessageSpy(&versionSocket, &QWebSocket::textMessageReceived);
    QSignalSpy versionDisconnectedSpy(&versionSocket, &QWebSocket::disconnected);
    versionSocket.open(plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::StateWebSocketServer::endpointPath()));
    QTRY_COMPARE(versionConnectedSpy.count(), 1);
    versionSocket.sendTextMessage(helloMessage(plasma_bridge::api::StateWebSocketServer::protocolVersion() + 1));
    QTRY_COMPARE(versionMessageSpy.count(), 1);
    QTRY_COMPARE(versionDisconnectedSpy.count(), 1);
    QCOMPARE(errorObject(takeMessage(versionMessageSpy)).value(QStringLiteral("code")).toString(),
             QStringLiteral("unsupported_protocol_version"));

    QWebSocket binarySocket;
    QSignalSpy binaryConnectedSpy(&binarySocket, &QWebSocket::connected);
    QSignalSpy binaryMessageSpy(&binarySocket, &QWebSocket::textMessageReceived);
    QSignalSpy binaryDisconnectedSpy(&binarySocket, &QWebSocket::disconnected);
    binarySocket.open(plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::StateWebSocketServer::endpointPath()));
    QTRY_COMPARE(binaryConnectedSpy.count(), 1);
    binarySocket.sendBinaryMessage(QByteArrayLiteral("abc"));
    QTRY_COMPARE(binaryMessageSpy.count(), 1);
    QTRY_COMPARE(binaryDisconnectedSpy.count(), 1);
    QCOMPARE(errorObject(takeMessage(binaryMessageSpy)).value(QStringLiteral("code")).toString(), QStringLiteral("bad_request"));
}

void StateWebSocketServerFeatureTest::sendsNotReadyThenFullStateAfterReadiness()
{
    plasma_bridge::state::AudioStateStore store;
    plasma_bridge::api::StateWebSocketServer server(&store);
    QVERIFY(server.listen(bindAddress(), 0));

    QWebSocket socket;
    QSignalSpy connectedSpy(&socket, &QWebSocket::connected);
    QSignalSpy messageSpy(&socket, &QWebSocket::textMessageReceived);
    QSignalSpy disconnectedSpy(&socket, &QWebSocket::disconnected);

    socket.open(plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::StateWebSocketServer::endpointPath()));
    QTRY_COMPARE(connectedSpy.count(), 1);
    socket.sendTextMessage(helloMessage());

    QTRY_COMPARE(messageSpy.count(), 1);
    {
        const QJsonObject error = takeMessage(messageSpy);
        QCOMPARE(error.value(QStringLiteral("type")).toString(), QStringLiteral("error"));
        QVERIFY(error.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(error).value(QStringLiteral("code")).toString(), QStringLiteral("not_ready"));
    }
    QCOMPARE(disconnectedSpy.count(), 0);

    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    QTRY_COMPARE(messageSpy.count(), 1);
    const QJsonObject fullState = takeMessage(messageSpy);
    QCOMPARE(fullState.value(QStringLiteral("type")).toString(), QStringLiteral("fullState"));
    QVERIFY(fullState.value(QStringLiteral("error")).isNull());
    const QJsonObject audio = payloadObject(fullState).value(QStringLiteral("audio")).toObject();
    QCOMPARE(audio.value(QStringLiteral("sources")).toArray().size(), store.audioState().sources.size());
    QCOMPARE(audio.value(QStringLiteral("selectedSourceId")).toString(), store.audioState().selectedSourceId);

    socket.close();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
}

void StateWebSocketServerFeatureTest::sendsPatchMessagesToAllReadyClients()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::api::StateWebSocketServer server(&store);
    QVERIFY(server.listen(bindAddress(), 0));

    QWebSocket firstSocket;
    QWebSocket secondSocket;
    QSignalSpy firstConnectedSpy(&firstSocket, &QWebSocket::connected);
    QSignalSpy secondConnectedSpy(&secondSocket, &QWebSocket::connected);
    QSignalSpy firstMessageSpy(&firstSocket, &QWebSocket::textMessageReceived);
    QSignalSpy secondMessageSpy(&secondSocket, &QWebSocket::textMessageReceived);

    const QUrl url = plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::StateWebSocketServer::endpointPath());
    firstSocket.open(url);
    secondSocket.open(url);

    QTRY_COMPARE(firstConnectedSpy.count(), 1);
    QTRY_COMPARE(secondConnectedSpy.count(), 1);

    firstSocket.sendTextMessage(helloMessage());
    secondSocket.sendTextMessage(helloMessage());

    QTRY_COMPARE(firstMessageSpy.count(), 1);
    QTRY_COMPARE(secondMessageSpy.count(), 1);
    const QJsonObject firstFullState = takeMessage(firstMessageSpy);
    const QJsonObject secondFullState = takeMessage(secondMessageSpy);
    QCOMPARE(firstFullState.value(QStringLiteral("type")).toString(), QStringLiteral("fullState"));
    QCOMPARE(secondFullState.value(QStringLiteral("type")).toString(), QStringLiteral("fullState"));
    QVERIFY(firstFullState.value(QStringLiteral("error")).isNull());
    QVERIFY(secondFullState.value(QStringLiteral("error")).isNull());
    QCOMPARE(payloadObject(firstFullState).value(QStringLiteral("audio")).toObject().value(QStringLiteral("selectedSourceId")).toString(),
             store.audioState().selectedSourceId);
    QCOMPARE(payloadObject(secondFullState).value(QStringLiteral("audio")).toObject().value(QStringLiteral("sources")).toArray().size(),
             store.audioState().sources.size());

    store.updateAudioState(plasma_bridge::tests::alternateAudioState(),
                           true,
                           QStringLiteral("sink-updated"),
                           QStringLiteral("bluez_output.headset.1"),
                           QString());

    QTRY_COMPARE(firstMessageSpy.count(), 1);
    QTRY_COMPARE(secondMessageSpy.count(), 1);

    const QJsonObject firstPatch = takeMessage(firstMessageSpy);
    const QJsonObject secondPatch = takeMessage(secondMessageSpy);
    QCOMPARE(firstPatch.value(QStringLiteral("type")).toString(), QStringLiteral("patch"));
    QCOMPARE(secondPatch.value(QStringLiteral("type")).toString(), QStringLiteral("patch"));
    QVERIFY(firstPatch.value(QStringLiteral("error")).isNull());
    QVERIFY(secondPatch.value(QStringLiteral("error")).isNull());
    QCOMPARE(payloadObject(firstPatch).value(QStringLiteral("reason")).toString(), QStringLiteral("sink-updated"));
    QCOMPARE(payloadObject(secondPatch).value(QStringLiteral("reason")).toString(), QStringLiteral("sink-updated"));
    QCOMPARE(payloadObject(firstPatch).value(QStringLiteral("sinkId")).toString(), QStringLiteral("bluez_output.headset.1"));
    QVERIFY(payloadObject(firstPatch).value(QStringLiteral("sourceId")).isNull());
    QVERIFY(payloadObject(firstPatch).value(QStringLiteral("windowId")).isNull());
    QVERIFY(payloadObject(secondPatch).value(QStringLiteral("sourceId")).isNull());

    plasma_bridge::AudioState sourceUpdatedState = plasma_bridge::tests::alternateAudioState();
    sourceUpdatedState.sources[1].volume = 0.77;
    store.updateAudioState(sourceUpdatedState,
                           true,
                           QStringLiteral("source-updated"),
                           QString(),
                           QStringLiteral("bluez_input.headset.1"));

    QTRY_COMPARE(firstMessageSpy.count(), 1);
    QTRY_COMPARE(secondMessageSpy.count(), 1);

    const QJsonObject firstSourcePatch = takeMessage(firstMessageSpy);
    const QJsonObject secondSourcePatch = takeMessage(secondMessageSpy);
    QCOMPARE(firstSourcePatch.value(QStringLiteral("type")).toString(), QStringLiteral("patch"));
    QCOMPARE(secondSourcePatch.value(QStringLiteral("type")).toString(), QStringLiteral("patch"));
    QCOMPARE(payloadObject(firstSourcePatch).value(QStringLiteral("reason")).toString(), QStringLiteral("source-updated"));
    QVERIFY(payloadObject(firstSourcePatch).value(QStringLiteral("sinkId")).isNull());
    QCOMPARE(payloadObject(firstSourcePatch).value(QStringLiteral("sourceId")).toString(), QStringLiteral("bluez_input.headset.1"));
    QVERIFY(payloadObject(firstSourcePatch).value(QStringLiteral("windowId")).isNull());
    QCOMPARE(payloadObject(secondSourcePatch).value(QStringLiteral("sourceId")).toString(), QStringLiteral("bluez_input.headset.1"));

    store.updateAudioState(plasma_bridge::tests::sampleAudioState(),
                           true,
                           QStringLiteral("default-source-changed"),
                           QString(),
                           QStringLiteral("alsa_input.usb-default.analog-stereo"));

    QTRY_COMPARE(firstMessageSpy.count(), 1);
    const QJsonObject defaultSourcePatch = takeMessage(firstMessageSpy);
    QCOMPARE(defaultSourcePatch.value(QStringLiteral("type")).toString(), QStringLiteral("patch"));
    QCOMPARE(payloadObject(defaultSourcePatch).value(QStringLiteral("reason")).toString(), QStringLiteral("default-source-changed"));
    QVERIFY(payloadObject(defaultSourcePatch).value(QStringLiteral("sinkId")).isNull());
    QCOMPARE(payloadObject(defaultSourcePatch).value(QStringLiteral("sourceId")).toString(),
             QStringLiteral("alsa_input.usb-default.analog-stereo"));
    QVERIFY(payloadObject(defaultSourcePatch).value(QStringLiteral("windowId")).isNull());

    QSignalSpy firstDisconnectedSpy(&firstSocket, &QWebSocket::disconnected);
    QSignalSpy secondDisconnectedSpy(&secondSocket, &QWebSocket::disconnected);
    firstSocket.close();
    secondSocket.close();
    QTRY_COMPARE(firstDisconnectedSpy.count(), 1);
    QTRY_COMPARE(secondDisconnectedSpy.count(), 1);
}

void StateWebSocketServerFeatureTest::sendsMediaNotReadyThenFullStateAfterReadiness()
{
    plasma_bridge::state::AudioStateStore audioStore;
    audioStore.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));
    plasma_bridge::state::MediaStateStore mediaStore;

    plasma_bridge::api::StateWebSocketServer server(&audioStore, &mediaStore);
    QVERIFY(server.listen(bindAddress(), 0));

    QWebSocket socket;
    QSignalSpy connectedSpy(&socket, &QWebSocket::connected);
    QSignalSpy messageSpy(&socket, &QWebSocket::textMessageReceived);
    QSignalSpy disconnectedSpy(&socket, &QWebSocket::disconnected);

    socket.open(plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::StateWebSocketServer::endpointPath()));
    QTRY_COMPARE(connectedSpy.count(), 1);
    socket.sendTextMessage(helloMessage());

    QTRY_COMPARE(messageSpy.count(), 1);
    {
        const QJsonObject error = takeMessage(messageSpy);
        QCOMPARE(error.value(QStringLiteral("type")).toString(), QStringLiteral("error"));
        QCOMPARE(errorObject(error).value(QStringLiteral("code")).toString(), QStringLiteral("not_ready"));
    }
    QCOMPARE(disconnectedSpy.count(), 0);

    mediaStore.updateMediaState(plasma_bridge::tests::sampleMediaState(), true, QStringLiteral("initial"));

    QTRY_COMPARE(messageSpy.count(), 1);
    const QJsonObject fullState = takeMessage(messageSpy);
    QCOMPARE(fullState.value(QStringLiteral("type")).toString(), QStringLiteral("fullState"));
    QVERIFY(fullState.value(QStringLiteral("error")).isNull());
    QVERIFY(payloadObject(fullState).contains(QStringLiteral("audio")));
    const QJsonObject media = payloadObject(fullState).value(QStringLiteral("media")).toObject();
    QCOMPARE(media.value(QStringLiteral("player")).toObject().value(QStringLiteral("playerId")).toString(),
             QStringLiteral("org.mpris.MediaPlayer2.spotify"));
    QCOMPARE(media.value(QStringLiteral("player")).toObject().value(QStringLiteral("positionMs")).toInteger(), 64000);
    QCOMPARE(media.value(QStringLiteral("player")).toObject().value(QStringLiteral("canSeek")).toBool(), true);

    socket.close();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
}

void StateWebSocketServerFeatureTest::sendsMediaPatchMessagesToAllReadyClients()
{
    plasma_bridge::state::AudioStateStore audioStore;
    audioStore.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));
    plasma_bridge::state::MediaStateStore mediaStore;
    mediaStore.updateMediaState(plasma_bridge::tests::sampleMediaState(), true, QStringLiteral("initial"));

    plasma_bridge::api::StateWebSocketServer server(&audioStore, &mediaStore);
    QVERIFY(server.listen(bindAddress(), 0));

    QWebSocket firstSocket;
    QWebSocket secondSocket;
    QSignalSpy firstConnectedSpy(&firstSocket, &QWebSocket::connected);
    QSignalSpy secondConnectedSpy(&secondSocket, &QWebSocket::connected);
    QSignalSpy firstMessageSpy(&firstSocket, &QWebSocket::textMessageReceived);
    QSignalSpy secondMessageSpy(&secondSocket, &QWebSocket::textMessageReceived);

    const QUrl url = plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::StateWebSocketServer::endpointPath());
    firstSocket.open(url);
    secondSocket.open(url);

    QTRY_COMPARE(firstConnectedSpy.count(), 1);
    QTRY_COMPARE(secondConnectedSpy.count(), 1);

    firstSocket.sendTextMessage(helloMessage());
    secondSocket.sendTextMessage(helloMessage());

    QTRY_COMPARE(firstMessageSpy.count(), 1);
    QTRY_COMPARE(secondMessageSpy.count(), 1);
    const QJsonObject firstFullState = takeMessage(firstMessageSpy);
    const QJsonObject secondFullState = takeMessage(secondMessageSpy);
    QVERIFY(payloadObject(firstFullState).contains(QStringLiteral("media")));
    QVERIFY(payloadObject(secondFullState).contains(QStringLiteral("media")));

    mediaStore.updateMediaState(plasma_bridge::tests::sampleMediaStateWithoutPlayer(),
                                true,
                                QStringLiteral("player-removed"),
                                QStringLiteral("org.mpris.MediaPlayer2.spotify"));

    QTRY_COMPARE(firstMessageSpy.count(), 1);
    QTRY_COMPARE(secondMessageSpy.count(), 1);

    const QJsonObject firstPatch = takeMessage(firstMessageSpy);
    const QJsonObject secondPatch = takeMessage(secondMessageSpy);
    QCOMPARE(firstPatch.value(QStringLiteral("type")).toString(), QStringLiteral("patch"));
    QCOMPARE(payloadObject(firstPatch).value(QStringLiteral("reason")).toString(), QStringLiteral("player-removed"));
    QCOMPARE(payloadObject(firstPatch).value(QStringLiteral("playerId")).toString(),
             QStringLiteral("org.mpris.MediaPlayer2.spotify"));
    const QJsonObject change = payloadObject(firstPatch).value(QStringLiteral("changes")).toArray().at(0).toObject();
    QCOMPARE(change.value(QStringLiteral("path")).toString(), QStringLiteral("media"));
    QVERIFY(change.value(QStringLiteral("value")).toObject().value(QStringLiteral("player")).isNull());
    QCOMPARE(payloadObject(secondPatch).value(QStringLiteral("playerId")).toString(),
             QStringLiteral("org.mpris.MediaPlayer2.spotify"));

    plasma_bridge::MediaState progressedState = plasma_bridge::tests::sampleMediaState();
    progressedState.player->positionMs = 71000;
    mediaStore.updateMediaState(progressedState,
                                true,
                                QStringLiteral("position-updated"),
                                QStringLiteral("org.mpris.MediaPlayer2.spotify"));

    QTRY_COMPARE(firstMessageSpy.count(), 1);
    const QJsonObject progressPatch = takeMessage(firstMessageSpy);
    QCOMPARE(progressPatch.value(QStringLiteral("type")).toString(), QStringLiteral("patch"));
    QCOMPARE(payloadObject(progressPatch).value(QStringLiteral("reason")).toString(), QStringLiteral("position-updated"));
    const QJsonObject progressChange = payloadObject(progressPatch).value(QStringLiteral("changes")).toArray().at(0).toObject();
    QCOMPARE(progressChange.value(QStringLiteral("path")).toString(), QStringLiteral("media"));
    QCOMPARE(progressChange.value(QStringLiteral("value")).toObject().value(QStringLiteral("player")).toObject().value(QStringLiteral("positionMs")).toInteger(),
             71000);

    QSignalSpy firstDisconnectedSpy(&firstSocket, &QWebSocket::disconnected);
    QSignalSpy secondDisconnectedSpy(&secondSocket, &QWebSocket::disconnected);
    firstSocket.close();
    secondSocket.close();
    QTRY_COMPARE(firstDisconnectedSpy.count(), 1);
    QTRY_COMPARE(secondDisconnectedSpy.count(), 1);
}

void StateWebSocketServerFeatureTest::sendsWindowNotReadyThenFullStateAfterReadiness()
{
    plasma_bridge::state::AudioStateStore audioStore;
    audioStore.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));
    plasma_bridge::state::WindowStateStore windowStore;

    plasma_bridge::api::StateWebSocketServer server(&audioStore, &windowStore);
    QVERIFY(server.listen(bindAddress(), 0));

    QWebSocket socket;
    QSignalSpy connectedSpy(&socket, &QWebSocket::connected);
    QSignalSpy messageSpy(&socket, &QWebSocket::textMessageReceived);
    QSignalSpy disconnectedSpy(&socket, &QWebSocket::disconnected);

    socket.open(plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::StateWebSocketServer::endpointPath()));
    QTRY_COMPARE(connectedSpy.count(), 1);
    socket.sendTextMessage(helloMessage());

    QTRY_COMPARE(messageSpy.count(), 1);
    {
        const QJsonObject error = takeMessage(messageSpy);
        QCOMPARE(error.value(QStringLiteral("type")).toString(), QStringLiteral("error"));
        QVERIFY(error.value(QStringLiteral("payload")).isNull());
        QCOMPARE(errorObject(error).value(QStringLiteral("code")).toString(), QStringLiteral("not_ready"));
    }
    QCOMPARE(disconnectedSpy.count(), 0);

    windowStore.updateWindowState(plasma_bridge::tests::sampleWindowSnapshot(), true, QStringLiteral("initial"));

    QTRY_COMPARE(messageSpy.count(), 1);
    const QJsonObject fullState = takeMessage(messageSpy);
    QCOMPARE(fullState.value(QStringLiteral("type")).toString(), QStringLiteral("fullState"));
    QVERIFY(fullState.value(QStringLiteral("error")).isNull());
    const QJsonObject audio = payloadObject(fullState).value(QStringLiteral("audio")).toObject();
    QCOMPARE(audio.value(QStringLiteral("selectedSourceId")).toString(), audioStore.audioState().selectedSourceId);
    const QJsonObject windowState = payloadObject(fullState).value(QStringLiteral("windowState")).toObject();
    QCOMPARE(windowState.value(QStringLiteral("activeWindowId")).toString(), QStringLiteral("window-editor"));
    QCOMPARE(windowState.value(QStringLiteral("activeWindow")).toObject().value(QStringLiteral("id")).toString(),
             QStringLiteral("window-editor"));
    QCOMPARE(windowState.value(QStringLiteral("activeWindow")).toObject().value(QStringLiteral("iconUrl")).toString(),
             QStringLiteral("/icons/apps/org.kde.kate"));
    QCOMPARE(windowState.value(QStringLiteral("windows")).toArray().size(), 2);

    socket.close();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
}

void StateWebSocketServerFeatureTest::sendsWindowPatchMessagesToAllReadyClients()
{
    plasma_bridge::state::AudioStateStore audioStore;
    audioStore.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));
    plasma_bridge::state::WindowStateStore windowStore;
    windowStore.updateWindowState(plasma_bridge::tests::sampleWindowSnapshot(), true, QStringLiteral("initial"));

    plasma_bridge::api::StateWebSocketServer server(&audioStore, &windowStore);
    QVERIFY(server.listen(bindAddress(), 0));

    QWebSocket firstSocket;
    QWebSocket secondSocket;
    QSignalSpy firstConnectedSpy(&firstSocket, &QWebSocket::connected);
    QSignalSpy secondConnectedSpy(&secondSocket, &QWebSocket::connected);
    QSignalSpy firstMessageSpy(&firstSocket, &QWebSocket::textMessageReceived);
    QSignalSpy secondMessageSpy(&secondSocket, &QWebSocket::textMessageReceived);

    const QUrl url = plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::StateWebSocketServer::endpointPath());
    firstSocket.open(url);
    secondSocket.open(url);

    QTRY_COMPARE(firstConnectedSpy.count(), 1);
    QTRY_COMPARE(secondConnectedSpy.count(), 1);

    firstSocket.sendTextMessage(helloMessage());
    secondSocket.sendTextMessage(helloMessage());

    QTRY_COMPARE(firstMessageSpy.count(), 1);
    QTRY_COMPARE(secondMessageSpy.count(), 1);
    const QJsonObject firstFullState = takeMessage(firstMessageSpy);
    const QJsonObject secondFullState = takeMessage(secondMessageSpy);
    QCOMPARE(firstFullState.value(QStringLiteral("type")).toString(), QStringLiteral("fullState"));
    QCOMPARE(secondFullState.value(QStringLiteral("type")).toString(), QStringLiteral("fullState"));
    QVERIFY(payloadObject(firstFullState).contains(QStringLiteral("audio")));
    QVERIFY(payloadObject(firstFullState).contains(QStringLiteral("windowState")));
    QVERIFY(payloadObject(secondFullState).contains(QStringLiteral("audio")));
    QVERIFY(payloadObject(secondFullState).contains(QStringLiteral("windowState")));

    plasma_bridge::WindowSnapshot updatedSnapshot = plasma_bridge::tests::sampleWindowSnapshot();
    updatedSnapshot.activeWindowId = QStringLiteral("window-terminal");
    windowStore.updateWindowState(updatedSnapshot,
                                  true,
                                  QStringLiteral("active-window-changed"),
                                  QStringLiteral("window-terminal"));

    QTRY_COMPARE(firstMessageSpy.count(), 1);
    QTRY_COMPARE(secondMessageSpy.count(), 1);

    const QJsonObject firstPatch = takeMessage(firstMessageSpy);
    const QJsonObject secondPatch = takeMessage(secondMessageSpy);
    QCOMPARE(firstPatch.value(QStringLiteral("type")).toString(), QStringLiteral("patch"));
    QCOMPARE(secondPatch.value(QStringLiteral("type")).toString(), QStringLiteral("patch"));
    QVERIFY(firstPatch.value(QStringLiteral("error")).isNull());
    QVERIFY(secondPatch.value(QStringLiteral("error")).isNull());
    const QJsonObject payload = payloadObject(firstPatch);
    QCOMPARE(payload.value(QStringLiteral("reason")).toString(), QStringLiteral("active-window-changed"));
    QVERIFY(payload.value(QStringLiteral("sinkId")).isNull());
    QVERIFY(payload.value(QStringLiteral("sourceId")).isNull());
    QCOMPARE(payload.value(QStringLiteral("windowId")).toString(), QStringLiteral("window-terminal"));
    const QJsonObject change = payload.value(QStringLiteral("changes")).toArray().at(0).toObject();
    QCOMPARE(change.value(QStringLiteral("path")).toString(), QStringLiteral("windowState"));
    QCOMPARE(change.value(QStringLiteral("value")).toObject().value(QStringLiteral("activeWindowId")).toString(),
             QStringLiteral("window-terminal"));
    QCOMPARE(payloadObject(secondPatch).value(QStringLiteral("windowId")).toString(), QStringLiteral("window-terminal"));

    QSignalSpy firstDisconnectedSpy(&firstSocket, &QWebSocket::disconnected);
    QSignalSpy secondDisconnectedSpy(&secondSocket, &QWebSocket::disconnected);
    firstSocket.close();
    secondSocket.close();
    QTRY_COMPARE(firstDisconnectedSpy.count(), 1);
    QTRY_COMPARE(secondDisconnectedSpy.count(), 1);
}

QJsonObject StateWebSocketServerFeatureTest::takeMessage(QSignalSpy &spy, const int index)
{
    return plasma_bridge::tests::parseJsonObject(spy.takeAt(index).at(0).toString().toUtf8());
}

QHostAddress StateWebSocketServerFeatureTest::bindAddress()
{
    return QHostAddress(QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST));
}

QString StateWebSocketServerFeatureTest::helloMessage(const int protocolVersion)
{
    return QStringLiteral("{\"type\":\"hello\",\"payload\":{\"protocolVersion\":%1},\"error\":null}")
        .arg(QString::number(protocolVersion));
}

QTEST_GUILESS_MAIN(StateWebSocketServerFeatureTest)

#include "test_state_websocket_server.moc"
