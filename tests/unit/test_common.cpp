#include "common/audio_state.h"
#include "tests/support/test_support.h"

#include <QJsonArray>
#include <QtTest>

class CommonAudioStateTest : public QObject
{
    Q_OBJECT

private slots:
    void sinkJsonUsesNullForEmptyBackend();
    void audioStateJsonUsesNullForEmptySelectedSink();
    void eventJsonCarriesReasonAndNullSinkId();
    void humanReadableEventIncludesImportantFields();
};

void CommonAudioStateTest::sinkJsonUsesNullForEmptyBackend()
{
    plasma_bridge::AudioSinkState sink;
    sink.id = QStringLiteral("sink-1");
    sink.label = QStringLiteral("Desk Speakers");
    sink.volume = 0.61;
    sink.muted = true;
    sink.available = false;
    sink.isDefault = true;
    sink.isVirtual = true;

    const QJsonObject json = plasma_bridge::toJsonObject(sink);

    QCOMPARE(json.value(QStringLiteral("id")).toString(), sink.id);
    QCOMPARE(json.value(QStringLiteral("label")).toString(), sink.label);
    QCOMPARE(json.value(QStringLiteral("volume")).toDouble(), sink.volume);
    QCOMPARE(json.value(QStringLiteral("muted")).toBool(), sink.muted);
    QCOMPARE(json.value(QStringLiteral("available")).toBool(), sink.available);
    QCOMPARE(json.value(QStringLiteral("isDefault")).toBool(), sink.isDefault);
    QCOMPARE(json.value(QStringLiteral("isVirtual")).toBool(), sink.isVirtual);
    QVERIFY(json.value(QStringLiteral("backendApi")).isNull());
}

void CommonAudioStateTest::audioStateJsonUsesNullForEmptySelectedSink()
{
    plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    state.selectedSinkId.clear();

    const QJsonObject json = plasma_bridge::toJsonObject(state);
    const QJsonArray sinks = json.value(QStringLiteral("sinks")).toArray();

    QCOMPARE(sinks.size(), state.sinks.size());
    QVERIFY(json.value(QStringLiteral("selectedSinkId")).isNull());
}

void CommonAudioStateTest::eventJsonCarriesReasonAndNullSinkId()
{
    const plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    const QJsonObject json = plasma_bridge::toJsonEventObject(QStringLiteral("initial"), QString(), state);

    QCOMPARE(json.value(QStringLiteral("event")).toString(), QStringLiteral("audioState"));
    QCOMPARE(json.value(QStringLiteral("reason")).toString(), QStringLiteral("initial"));
    QVERIFY(json.value(QStringLiteral("sinkId")).isNull());
    QCOMPARE(json.value(QStringLiteral("state")).toObject().value(QStringLiteral("selectedSinkId")).toString(),
             state.selectedSinkId);
}

void CommonAudioStateTest::humanReadableEventIncludesImportantFields()
{
    const plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    const QString text = plasma_bridge::formatHumanReadableEvent(QStringLiteral("sink-updated"),
                                                                 state.selectedSinkId,
                                                                 state);

    QVERIFY(text.contains(QStringLiteral("Event: audioState")));
    QVERIFY(text.contains(QStringLiteral("Reason: sink-updated")));
    QVERIFY(text.contains(QStringLiteral("Selected Sink: %1").arg(state.selectedSinkId)));
    QVERIFY(text.contains(QStringLiteral("* USB Audio")));
    QVERIFY(text.contains(QStringLiteral("backendApi: alsa")));
}

QTEST_GUILESS_MAIN(CommonAudioStateTest)

#include "test_common.moc"
