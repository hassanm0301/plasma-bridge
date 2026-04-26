#include "common/audio_state.h"
#include "tests/support/test_support.h"

#include <QJsonArray>
#include <QtTest>

class CommonAudioStateTest : public QObject
{
    Q_OBJECT

private slots:
    void deviceJsonUsesNullForEmptyBackend();
    void outputStateJsonUsesNullForEmptySelectedSink();
    void inputStateJsonUsesNullForEmptySelectedSource();
    void fullAudioStateJsonIncludesSources();
    void eventJsonCarriesReasonAndNullDeviceIds();
    void humanReadableEventIncludesImportantFields();
};

void CommonAudioStateTest::deviceJsonUsesNullForEmptyBackend()
{
    plasma_bridge::AudioDeviceState device;
    device.id = QStringLiteral("device-1");
    device.label = QStringLiteral("Desk Speakers");
    device.volume = 0.61;
    device.muted = true;
    device.available = false;
    device.isDefault = true;
    device.isVirtual = true;

    const QJsonObject json = plasma_bridge::toJsonObject(device);

    QCOMPARE(json.value(QStringLiteral("id")).toString(), device.id);
    QCOMPARE(json.value(QStringLiteral("label")).toString(), device.label);
    QCOMPARE(json.value(QStringLiteral("volume")).toDouble(), device.volume);
    QCOMPARE(json.value(QStringLiteral("muted")).toBool(), device.muted);
    QCOMPARE(json.value(QStringLiteral("available")).toBool(), device.available);
    QCOMPARE(json.value(QStringLiteral("isDefault")).toBool(), device.isDefault);
    QCOMPARE(json.value(QStringLiteral("isVirtual")).toBool(), device.isVirtual);
    QVERIFY(json.value(QStringLiteral("backendApi")).isNull());
}

void CommonAudioStateTest::outputStateJsonUsesNullForEmptySelectedSink()
{
    plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    state.selectedSinkId.clear();

    const QJsonObject json = plasma_bridge::toJsonObject(plasma_bridge::toAudioOutputState(state));
    const QJsonArray sinks = json.value(QStringLiteral("sinks")).toArray();

    QCOMPARE(sinks.size(), state.sinks.size());
    QVERIFY(json.value(QStringLiteral("selectedSinkId")).isNull());
}

void CommonAudioStateTest::inputStateJsonUsesNullForEmptySelectedSource()
{
    plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    state.selectedSourceId.clear();

    const QJsonObject json = plasma_bridge::toJsonObject(plasma_bridge::toAudioInputState(state));
    const QJsonArray sources = json.value(QStringLiteral("sources")).toArray();

    QCOMPARE(sources.size(), state.sources.size());
    QVERIFY(json.value(QStringLiteral("selectedSourceId")).isNull());
}

void CommonAudioStateTest::fullAudioStateJsonIncludesSources()
{
    const plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    const QJsonObject json = plasma_bridge::toJsonObject(state);

    QCOMPARE(json.value(QStringLiteral("sinks")).toArray().size(), state.sinks.size());
    QCOMPARE(json.value(QStringLiteral("selectedSinkId")).toString(), state.selectedSinkId);
    QCOMPARE(json.value(QStringLiteral("sources")).toArray().size(), state.sources.size());
    QCOMPARE(json.value(QStringLiteral("selectedSourceId")).toString(), state.selectedSourceId);
}

void CommonAudioStateTest::eventJsonCarriesReasonAndNullDeviceIds()
{
    const plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    const QJsonObject json = plasma_bridge::toJsonEventObject(QStringLiteral("initial"), QString(), QString(), state);

    QCOMPARE(json.value(QStringLiteral("event")).toString(), QStringLiteral("audioState"));
    QCOMPARE(json.value(QStringLiteral("reason")).toString(), QStringLiteral("initial"));
    QVERIFY(json.value(QStringLiteral("sinkId")).isNull());
    QVERIFY(json.value(QStringLiteral("sourceId")).isNull());
    QCOMPARE(json.value(QStringLiteral("state")).toObject().value(QStringLiteral("selectedSinkId")).toString(),
             state.selectedSinkId);
    QCOMPARE(json.value(QStringLiteral("state")).toObject().value(QStringLiteral("selectedSourceId")).toString(),
             state.selectedSourceId);
}

void CommonAudioStateTest::humanReadableEventIncludesImportantFields()
{
    const plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    const QString text = plasma_bridge::formatHumanReadableEvent(QStringLiteral("sink-updated"),
                                                                 state.selectedSinkId,
                                                                 QString(),
                                                                 state);

    QVERIFY(text.contains(QStringLiteral("Event: audioState")));
    QVERIFY(text.contains(QStringLiteral("Reason: sink-updated")));
    QVERIFY(text.contains(QStringLiteral("Selected Sink: %1").arg(state.selectedSinkId)));
    QVERIFY(text.contains(QStringLiteral("Selected Source: %1").arg(state.selectedSourceId)));
    QVERIFY(text.contains(QStringLiteral("* USB Audio")));
    QVERIFY(text.contains(QStringLiteral("* USB Microphone")));
    QVERIFY(text.contains(QStringLiteral("backendApi: alsa")));
}

QTEST_GUILESS_MAIN(CommonAudioStateTest)

#include "test_common.moc"
