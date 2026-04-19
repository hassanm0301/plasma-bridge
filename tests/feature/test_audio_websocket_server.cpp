#include "api/audio_websocket_server.h"
#include "state/audio_state_store.h"
#include "tests/support/test_support.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QWebSocket>
#include <QtTest>

class AudioWebSocketServerFeatureTest : public QObject
{
    Q_OBJECT

private slots:
    void rejectsUnknownEndpoint();
    void ignoresStateChangesUntilHello();
    void rejectsMalformedFirstMessages();
    void rejectsUnsupportedProtocolVersionAndBinaryMessages();
    void sendsNotReadyThenFullStateAfterReadiness();
    void sendsPatchMessagesToAllReadyClients();

private:
    static QHostAddress bindAddress();
    static QJsonObject takeMessage(QSignalSpy &spy, int index = 0);
};

void AudioWebSocketServerFeatureTest::rejectsUnknownEndpoint()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::api::AudioWebSocketServer server(&store);
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
    QCOMPARE(error.value(QStringLiteral("code")).toString(), QStringLiteral("unknown_endpoint"));
}

void AudioWebSocketServerFeatureTest::ignoresStateChangesUntilHello()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::api::AudioWebSocketServer server(&store);
    QVERIFY(server.listen(bindAddress(), 0));

    QWebSocket socket;
    QSignalSpy connectedSpy(&socket, &QWebSocket::connected);
    QSignalSpy messageSpy(&socket, &QWebSocket::textMessageReceived);
    QSignalSpy disconnectedSpy(&socket, &QWebSocket::disconnected);

    socket.open(plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::AudioWebSocketServer::endpointPath()));
    QTRY_COMPARE(connectedSpy.count(), 1);

    store.updateAudioState(plasma_bridge::tests::alternateAudioState(), true, QStringLiteral("sink-updated"), QStringLiteral("bluez_output.headset.1"));
    QTest::qWait(50);
    QCOMPARE(messageSpy.count(), 0);

    socket.sendTextMessage(QStringLiteral("{\"type\":\"hello\",\"protocolVersion\":1}"));
    QTRY_COMPARE(messageSpy.count(), 1);
    QCOMPARE(takeMessage(messageSpy).value(QStringLiteral("type")).toString(), QStringLiteral("fullState"));

    socket.close();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
}

void AudioWebSocketServerFeatureTest::rejectsMalformedFirstMessages()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::api::AudioWebSocketServer server(&store);
    QVERIFY(server.listen(bindAddress(), 0));

    QWebSocket malformedSocket;
    QSignalSpy malformedConnectedSpy(&malformedSocket, &QWebSocket::connected);
    QSignalSpy malformedMessageSpy(&malformedSocket, &QWebSocket::textMessageReceived);
    QSignalSpy malformedDisconnectedSpy(&malformedSocket, &QWebSocket::disconnected);
    malformedSocket.open(plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::AudioWebSocketServer::endpointPath()));
    QTRY_COMPARE(malformedConnectedSpy.count(), 1);
    malformedSocket.sendTextMessage(QStringLiteral("{"));
    QTRY_COMPARE(malformedMessageSpy.count(), 1);
    QTRY_COMPARE(malformedDisconnectedSpy.count(), 1);
    QCOMPARE(takeMessage(malformedMessageSpy).value(QStringLiteral("code")).toString(), QStringLiteral("bad_request"));

    QWebSocket wrongTypeSocket;
    QSignalSpy wrongTypeConnectedSpy(&wrongTypeSocket, &QWebSocket::connected);
    QSignalSpy wrongTypeMessageSpy(&wrongTypeSocket, &QWebSocket::textMessageReceived);
    QSignalSpy wrongTypeDisconnectedSpy(&wrongTypeSocket, &QWebSocket::disconnected);
    wrongTypeSocket.open(plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::AudioWebSocketServer::endpointPath()));
    QTRY_COMPARE(wrongTypeConnectedSpy.count(), 1);
    wrongTypeSocket.sendTextMessage(QStringLiteral("{\"type\":\"patch\"}"));
    QTRY_COMPARE(wrongTypeMessageSpy.count(), 1);
    QTRY_COMPARE(wrongTypeDisconnectedSpy.count(), 1);
    QCOMPARE(takeMessage(wrongTypeMessageSpy).value(QStringLiteral("code")).toString(), QStringLiteral("bad_request"));
}

void AudioWebSocketServerFeatureTest::rejectsUnsupportedProtocolVersionAndBinaryMessages()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::api::AudioWebSocketServer server(&store);
    QVERIFY(server.listen(bindAddress(), 0));

    QWebSocket versionSocket;
    QSignalSpy versionConnectedSpy(&versionSocket, &QWebSocket::connected);
    QSignalSpy versionMessageSpy(&versionSocket, &QWebSocket::textMessageReceived);
    QSignalSpy versionDisconnectedSpy(&versionSocket, &QWebSocket::disconnected);
    versionSocket.open(plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::AudioWebSocketServer::endpointPath()));
    QTRY_COMPARE(versionConnectedSpy.count(), 1);
    versionSocket.sendTextMessage(QStringLiteral("{\"type\":\"hello\",\"protocolVersion\":2}"));
    QTRY_COMPARE(versionMessageSpy.count(), 1);
    QTRY_COMPARE(versionDisconnectedSpy.count(), 1);
    QCOMPARE(takeMessage(versionMessageSpy).value(QStringLiteral("code")).toString(),
             QStringLiteral("unsupported_protocol_version"));

    QWebSocket binarySocket;
    QSignalSpy binaryConnectedSpy(&binarySocket, &QWebSocket::connected);
    QSignalSpy binaryMessageSpy(&binarySocket, &QWebSocket::textMessageReceived);
    QSignalSpy binaryDisconnectedSpy(&binarySocket, &QWebSocket::disconnected);
    binarySocket.open(plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::AudioWebSocketServer::endpointPath()));
    QTRY_COMPARE(binaryConnectedSpy.count(), 1);
    binarySocket.sendBinaryMessage(QByteArrayLiteral("abc"));
    QTRY_COMPARE(binaryMessageSpy.count(), 1);
    QTRY_COMPARE(binaryDisconnectedSpy.count(), 1);
    QCOMPARE(takeMessage(binaryMessageSpy).value(QStringLiteral("code")).toString(), QStringLiteral("bad_request"));
}

void AudioWebSocketServerFeatureTest::sendsNotReadyThenFullStateAfterReadiness()
{
    plasma_bridge::state::AudioStateStore store;
    plasma_bridge::api::AudioWebSocketServer server(&store);
    QVERIFY(server.listen(bindAddress(), 0));

    QWebSocket socket;
    QSignalSpy connectedSpy(&socket, &QWebSocket::connected);
    QSignalSpy messageSpy(&socket, &QWebSocket::textMessageReceived);
    QSignalSpy disconnectedSpy(&socket, &QWebSocket::disconnected);

    socket.open(plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::AudioWebSocketServer::endpointPath()));
    QTRY_COMPARE(connectedSpy.count(), 1);
    socket.sendTextMessage(QStringLiteral("{\"type\":\"hello\",\"protocolVersion\":1}"));

    QTRY_COMPARE(messageSpy.count(), 1);
    QCOMPARE(takeMessage(messageSpy).value(QStringLiteral("code")).toString(), QStringLiteral("not_ready"));
    QCOMPARE(disconnectedSpy.count(), 0);

    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    QTRY_COMPARE(messageSpy.count(), 1);
    QCOMPARE(takeMessage(messageSpy).value(QStringLiteral("type")).toString(), QStringLiteral("fullState"));

    socket.close();
    QTRY_COMPARE(disconnectedSpy.count(), 1);
}

void AudioWebSocketServerFeatureTest::sendsPatchMessagesToAllReadyClients()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), true, QStringLiteral("initial"));

    plasma_bridge::api::AudioWebSocketServer server(&store);
    QVERIFY(server.listen(bindAddress(), 0));

    QWebSocket firstSocket;
    QWebSocket secondSocket;
    QSignalSpy firstConnectedSpy(&firstSocket, &QWebSocket::connected);
    QSignalSpy secondConnectedSpy(&secondSocket, &QWebSocket::connected);
    QSignalSpy firstMessageSpy(&firstSocket, &QWebSocket::textMessageReceived);
    QSignalSpy secondMessageSpy(&secondSocket, &QWebSocket::textMessageReceived);

    const QUrl url = plasma_bridge::tests::wsUrl(server.serverPort(), plasma_bridge::api::AudioWebSocketServer::endpointPath());
    firstSocket.open(url);
    secondSocket.open(url);

    QTRY_COMPARE(firstConnectedSpy.count(), 1);
    QTRY_COMPARE(secondConnectedSpy.count(), 1);

    firstSocket.sendTextMessage(QStringLiteral("{\"type\":\"hello\",\"protocolVersion\":1}"));
    secondSocket.sendTextMessage(QStringLiteral("{\"type\":\"hello\",\"protocolVersion\":1}"));

    QTRY_COMPARE(firstMessageSpy.count(), 1);
    QTRY_COMPARE(secondMessageSpy.count(), 1);
    QCOMPARE(takeMessage(firstMessageSpy).value(QStringLiteral("type")).toString(), QStringLiteral("fullState"));
    QCOMPARE(takeMessage(secondMessageSpy).value(QStringLiteral("type")).toString(), QStringLiteral("fullState"));

    store.updateAudioState(plasma_bridge::tests::alternateAudioState(),
                           true,
                           QStringLiteral("sink-updated"),
                           QStringLiteral("bluez_output.headset.1"));

    QTRY_COMPARE(firstMessageSpy.count(), 1);
    QTRY_COMPARE(secondMessageSpy.count(), 1);

    const QJsonObject firstPatch = takeMessage(firstMessageSpy);
    const QJsonObject secondPatch = takeMessage(secondMessageSpy);
    QCOMPARE(firstPatch.value(QStringLiteral("type")).toString(), QStringLiteral("patch"));
    QCOMPARE(secondPatch.value(QStringLiteral("type")).toString(), QStringLiteral("patch"));
    QCOMPARE(firstPatch.value(QStringLiteral("reason")).toString(), QStringLiteral("sink-updated"));
    QCOMPARE(secondPatch.value(QStringLiteral("reason")).toString(), QStringLiteral("sink-updated"));

    store.updateAudioState(plasma_bridge::tests::sampleAudioState(),
                           true,
                           QStringLiteral("default-sink-changed"),
                           QStringLiteral("alsa_output.usb-default.analog-stereo"));

    QTRY_COMPARE(firstMessageSpy.count(), 1);
    QCOMPARE(takeMessage(firstMessageSpy).value(QStringLiteral("type")).toString(), QStringLiteral("patch"));

    QSignalSpy firstDisconnectedSpy(&firstSocket, &QWebSocket::disconnected);
    QSignalSpy secondDisconnectedSpy(&secondSocket, &QWebSocket::disconnected);
    firstSocket.close();
    secondSocket.close();
    QTRY_COMPARE(firstDisconnectedSpy.count(), 1);
    QTRY_COMPARE(secondDisconnectedSpy.count(), 1);
}

QJsonObject AudioWebSocketServerFeatureTest::takeMessage(QSignalSpy &spy, const int index)
{
    return plasma_bridge::tests::parseJsonObject(spy.takeAt(index).at(0).toString().toUtf8());
}

QHostAddress AudioWebSocketServerFeatureTest::bindAddress()
{
    return QHostAddress(QStringLiteral("127.0.0.1"));
}

QTEST_GUILESS_MAIN(AudioWebSocketServerFeatureTest)

#include "test_audio_websocket_server.moc"
