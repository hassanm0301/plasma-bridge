#include "state/audio_state_store.h"
#include "tests/support/test_support.h"

#include <QSignalSpy>
#include <QtTest>

class AudioStateStoreTest : public QObject
{
    Q_OBJECT

private slots:
    void updateAudioStateTransitionsReadinessAndSignals();
    void defaultSinkPrefersSelectedSinkId();
    void defaultSinkFallsBackToIsDefault();
    void defaultSinkRequiresReadyState();
};

void AudioStateStoreTest::updateAudioStateTransitionsReadinessAndSignals()
{
    plasma_bridge::state::AudioStateStore store;
    QSignalSpy readySpy(&store, &plasma_bridge::state::AudioStateStore::readyChanged);
    QSignalSpy stateSpy(&store, &plasma_bridge::state::AudioStateStore::audioStateChanged);

    const plasma_bridge::AudioState initialState = plasma_bridge::tests::sampleAudioState();
    store.updateAudioState(initialState, false, QStringLiteral("initial"));

    QCOMPARE(store.isReady(), false);
    QCOMPARE(readySpy.count(), 0);
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.takeFirst().at(0).toString(), QStringLiteral("initial"));

    const plasma_bridge::AudioState updatedState = plasma_bridge::tests::alternateAudioState();
    store.updateAudioState(updatedState, true, QStringLiteral("sink-updated"), updatedState.selectedSinkId);

    QCOMPARE(store.isReady(), true);
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(readySpy.takeFirst().at(0).toBool(), true);
    QCOMPARE(stateSpy.count(), 1);
    const QList<QVariant> stateArgs = stateSpy.takeFirst();
    QCOMPARE(stateArgs.at(0).toString(), QStringLiteral("sink-updated"));
    QCOMPARE(stateArgs.at(1).toString(), updatedState.selectedSinkId);
    QCOMPARE(store.audioState().selectedSinkId, updatedState.selectedSinkId);
}

void AudioStateStoreTest::defaultSinkPrefersSelectedSinkId()
{
    plasma_bridge::state::AudioStateStore store;
    plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    state.sinks[0].isDefault = false;
    state.sinks[1].isDefault = true;
    state.selectedSinkId = state.sinks[0].id;
    store.updateAudioState(state, true, QStringLiteral("initial"));

    const std::optional<plasma_bridge::AudioSinkState> defaultSink = store.defaultSink();

    QVERIFY(defaultSink.has_value());
    QCOMPARE(defaultSink->id, state.sinks[0].id);
}

void AudioStateStoreTest::defaultSinkFallsBackToIsDefault()
{
    plasma_bridge::state::AudioStateStore store;
    plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    state.selectedSinkId.clear();
    store.updateAudioState(state, true, QStringLiteral("initial"));

    const std::optional<plasma_bridge::AudioSinkState> defaultSink = store.defaultSink();

    QVERIFY(defaultSink.has_value());
    QCOMPARE(defaultSink->id, state.sinks[0].id);
}

void AudioStateStoreTest::defaultSinkRequiresReadyState()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), false, QStringLiteral("initial"));
    QVERIFY(!store.defaultSink().has_value());
}

QTEST_GUILESS_MAIN(AudioStateStoreTest)

#include "test_state.moc"
