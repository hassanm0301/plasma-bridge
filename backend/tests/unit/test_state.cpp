#include "state/audio_state_store.h"
#include "state/media_state_store.h"
#include "state/window_state_store.h"
#include "adapters/media/media_observer.h"
#include "adapters/window/window_observer.h"
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
    void defaultSourcePrefersSelectedSourceId();
    void defaultSourceFallsBackToIsDefault();
    void defaultSinkRequiresReadyState();
    void defaultSourceRequiresReadyState();
    void mediaStateStoreTransitionsReadinessAndSignals();
    void mediaStateStoreAttachesGenericObserver();
    void currentPlayerRequiresReadyMediaState();
    void updateWindowStateTransitionsReadinessAndNormalizesActiveWindow();
    void windowStateStoreAttachesGenericObserver();
    void activeWindowRequiresReadyWindowState();
};

namespace
{

class FakeWindowObserver final : public plasma_bridge::window::WindowObserver
{
    Q_OBJECT

public:
    using WindowObserver::WindowObserver;

    void start() override
    {
        ++m_startCount;
    }

    const plasma_bridge::WindowSnapshot &currentSnapshot() const override
    {
        return m_snapshot;
    }

    bool hasInitialSnapshot() const override
    {
        return m_ready;
    }

    QString backendName() const override
    {
        return QStringLiteral("fake-window-observer");
    }

    void emitInitialSnapshot(const plasma_bridge::WindowSnapshot &snapshot)
    {
        m_snapshot = snapshot;
        m_ready = true;
        emit initialSnapshotReady();
    }

    void emitWindowChange(const plasma_bridge::WindowSnapshot &snapshot,
                          const QString &reason,
                          const QString &windowId)
    {
        m_snapshot = snapshot;
        m_ready = true;
        emit windowStateChanged(reason, windowId);
    }

    int m_startCount = 0;
    plasma_bridge::WindowSnapshot m_snapshot;
    bool m_ready = false;
};

class FakeMediaObserver final : public plasma_bridge::media::MediaObserver
{
    Q_OBJECT

public:
    using MediaObserver::MediaObserver;

    void start() override
    {
    }

    const plasma_bridge::MediaState &currentState() const override
    {
        return m_state;
    }

    bool hasInitialState() const override
    {
        return m_ready;
    }

    void emitInitialMediaState(const plasma_bridge::MediaState &state)
    {
        m_state = state;
        m_ready = true;
        emit initialStateReady();
    }

    void emitMediaChange(const plasma_bridge::MediaState &state,
                         const QString &reason,
                         const QString &playerId)
    {
        m_state = state;
        m_ready = true;
        emit mediaStateChanged(reason, playerId);
    }

    plasma_bridge::MediaState m_state;
    bool m_ready = false;
};

} // namespace

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
    store.updateAudioState(updatedState, true, QStringLiteral("sink-updated"), updatedState.selectedSinkId, QString());

    QCOMPARE(store.isReady(), true);
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(readySpy.takeFirst().at(0).toBool(), true);
    QCOMPARE(stateSpy.count(), 1);
    const QList<QVariant> stateArgs = stateSpy.takeFirst();
    QCOMPARE(stateArgs.at(0).toString(), QStringLiteral("sink-updated"));
    QCOMPARE(stateArgs.at(1).toString(), updatedState.selectedSinkId);
    QCOMPARE(stateArgs.at(2).toString(), QString());
    QCOMPARE(store.audioState().selectedSinkId, updatedState.selectedSinkId);
    QCOMPARE(store.audioState().selectedSourceId, updatedState.selectedSourceId);
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

void AudioStateStoreTest::defaultSourcePrefersSelectedSourceId()
{
    plasma_bridge::state::AudioStateStore store;
    plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    state.sources[0].isDefault = false;
    state.sources[1].isDefault = true;
    state.selectedSourceId = state.sources[0].id;
    store.updateAudioState(state, true, QStringLiteral("initial"));

    const std::optional<plasma_bridge::AudioSourceState> defaultSource = store.defaultSource();

    QVERIFY(defaultSource.has_value());
    QCOMPARE(defaultSource->id, state.sources[0].id);
}

void AudioStateStoreTest::defaultSourceFallsBackToIsDefault()
{
    plasma_bridge::state::AudioStateStore store;
    plasma_bridge::AudioState state = plasma_bridge::tests::sampleAudioState();
    state.selectedSourceId.clear();
    store.updateAudioState(state, true, QStringLiteral("initial"));

    const std::optional<plasma_bridge::AudioSourceState> defaultSource = store.defaultSource();

    QVERIFY(defaultSource.has_value());
    QCOMPARE(defaultSource->id, state.sources[0].id);
}

void AudioStateStoreTest::defaultSinkRequiresReadyState()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), false, QStringLiteral("initial"));
    QVERIFY(!store.defaultSink().has_value());
}

void AudioStateStoreTest::defaultSourceRequiresReadyState()
{
    plasma_bridge::state::AudioStateStore store;
    store.updateAudioState(plasma_bridge::tests::sampleAudioState(), false, QStringLiteral("initial"));
    QVERIFY(!store.defaultSource().has_value());
}

void AudioStateStoreTest::mediaStateStoreTransitionsReadinessAndSignals()
{
    plasma_bridge::state::MediaStateStore store;
    QSignalSpy readySpy(&store, &plasma_bridge::state::MediaStateStore::readyChanged);
    QSignalSpy stateSpy(&store, &plasma_bridge::state::MediaStateStore::mediaStateChanged);

    const plasma_bridge::MediaState initial = plasma_bridge::tests::sampleMediaStateWithoutPlayer();
    store.updateMediaState(initial, false, QStringLiteral("initial"));

    QCOMPARE(store.isReady(), false);
    QCOMPARE(readySpy.count(), 0);
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.takeFirst().at(0).toString(), QStringLiteral("initial"));

    const plasma_bridge::MediaState updated = plasma_bridge::tests::sampleMediaState();
    store.updateMediaState(updated, true, QStringLiteral("player-updated"), updated.player->playerId);

    QCOMPARE(store.isReady(), true);
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(readySpy.takeFirst().at(0).toBool(), true);
    QCOMPARE(stateSpy.count(), 1);
    const QList<QVariant> args = stateSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), QStringLiteral("player-updated"));
    QCOMPARE(args.at(1).toString(), updated.player->playerId);
    QVERIFY(store.currentPlayer().has_value());
    QCOMPARE(store.currentPlayer()->playerId, updated.player->playerId);
}

void AudioStateStoreTest::mediaStateStoreAttachesGenericObserver()
{
    plasma_bridge::state::MediaStateStore store;
    FakeMediaObserver observer;
    QSignalSpy readySpy(&store, &plasma_bridge::state::MediaStateStore::readyChanged);
    QSignalSpy stateSpy(&store, &plasma_bridge::state::MediaStateStore::mediaStateChanged);

    store.attachObserver(&observer);
    observer.emitInitialMediaState(plasma_bridge::tests::sampleMediaState());

    QCOMPARE(store.isReady(), true);
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.takeFirst().at(0).toString(), QStringLiteral("initial"));

    observer.emitMediaChange(plasma_bridge::tests::sampleMediaStateWithoutPlayer(),
                             QStringLiteral("player-removed"),
                             QStringLiteral("org.mpris.MediaPlayer2.spotify"));

    QCOMPARE(stateSpy.count(), 1);
    const QList<QVariant> args = stateSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), QStringLiteral("player-removed"));
    QCOMPARE(args.at(1).toString(), QStringLiteral("org.mpris.MediaPlayer2.spotify"));
    QVERIFY(!store.currentPlayer().has_value());
}

void AudioStateStoreTest::currentPlayerRequiresReadyMediaState()
{
    plasma_bridge::state::MediaStateStore store;
    store.updateMediaState(plasma_bridge::tests::sampleMediaState(), false, QStringLiteral("initial"));
    QVERIFY(!store.currentPlayer().has_value());
}

void AudioStateStoreTest::updateWindowStateTransitionsReadinessAndNormalizesActiveWindow()
{
    plasma_bridge::state::WindowStateStore store;
    QSignalSpy readySpy(&store, &plasma_bridge::state::WindowStateStore::readyChanged);
    QSignalSpy stateSpy(&store, &plasma_bridge::state::WindowStateStore::windowStateChanged);

    plasma_bridge::WindowSnapshot snapshot = plasma_bridge::tests::sampleWindowSnapshot();
    snapshot.activeWindowId = QStringLiteral("window-terminal");
    snapshot.windows[0].isActive = false;
    snapshot.windows[1].isActive = true;

    store.updateWindowState(snapshot, true, QStringLiteral("active-window-changed"), QStringLiteral("window-terminal"));

    QCOMPARE(store.isReady(), true);
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(readySpy.takeFirst().at(0).toBool(), true);
    QCOMPARE(stateSpy.count(), 1);
    const QList<QVariant> stateArgs = stateSpy.takeFirst();
    QCOMPARE(stateArgs.at(0).toString(), QStringLiteral("active-window-changed"));
    QCOMPARE(stateArgs.at(1).toString(), QStringLiteral("window-terminal"));
    QCOMPARE(store.windowState().activeWindowId, QStringLiteral("window-terminal"));
    QCOMPARE(store.windowState().windows[0].isActive, true);
    QCOMPARE(store.windowState().windows[1].isActive, false);
    QVERIFY(store.activeWindow().has_value());
    QCOMPARE(store.activeWindow()->id, QStringLiteral("window-terminal"));
}

void AudioStateStoreTest::windowStateStoreAttachesGenericObserver()
{
    plasma_bridge::state::WindowStateStore store;
    FakeWindowObserver observer;
    QSignalSpy readySpy(&store, &plasma_bridge::state::WindowStateStore::readyChanged);
    QSignalSpy stateSpy(&store, &plasma_bridge::state::WindowStateStore::windowStateChanged);

    store.attachObserver(&observer);
    observer.emitInitialSnapshot(plasma_bridge::tests::sampleWindowSnapshot());

    QCOMPARE(store.isReady(), true);
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(stateSpy.takeFirst().at(0).toString(), QStringLiteral("initial"));

    plasma_bridge::WindowSnapshot changed = plasma_bridge::tests::sampleWindowSnapshotWithoutActiveWindow();
    observer.emitWindowChange(changed, QStringLiteral("active-window-changed"), QStringLiteral("window-editor"));

    QCOMPARE(stateSpy.count(), 1);
    const QList<QVariant> args = stateSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), QStringLiteral("active-window-changed"));
    QCOMPARE(args.at(1).toString(), QStringLiteral("window-editor"));
    QVERIFY(!store.activeWindow().has_value());
}

void AudioStateStoreTest::activeWindowRequiresReadyWindowState()
{
    plasma_bridge::state::WindowStateStore store;
    store.updateWindowState(plasma_bridge::tests::sampleWindowSnapshot(), false, QStringLiteral("initial"));
    QVERIFY(!store.activeWindow().has_value());
}

QTEST_GUILESS_MAIN(AudioStateStoreTest)

#include "test_state.moc"
