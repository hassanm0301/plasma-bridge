#include "tests/support/test_support.h"

#include "plasma_bridge_build_config.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QResource>

static void initDocsResources()
{
    Q_INIT_RESOURCE(docs_resources);
}

namespace plasma_bridge::tests
{

AudioState sampleAudioState()
{
    AudioState state;

    AudioSinkState defaultSink;
    defaultSink.id = QStringLiteral("alsa_output.usb-default.analog-stereo");
    defaultSink.label = QStringLiteral("USB Audio");
    defaultSink.volume = 0.75;
    defaultSink.muted = false;
    defaultSink.available = true;
    defaultSink.isDefault = true;
    defaultSink.isVirtual = false;
    defaultSink.backendApi = QStringLiteral("alsa");

    AudioSinkState secondarySink;
    secondarySink.id = QStringLiteral("bluez_output.headset.1");
    secondarySink.label = QStringLiteral("Bluetooth Headset");
    secondarySink.volume = 0.35;
    secondarySink.muted = true;
    secondarySink.available = true;
    secondarySink.isDefault = false;
    secondarySink.isVirtual = false;
    secondarySink.backendApi = QStringLiteral("bluez");

    AudioSourceState defaultSource;
    defaultSource.id = QStringLiteral("alsa_input.usb-default.analog-stereo");
    defaultSource.label = QStringLiteral("USB Microphone");
    defaultSource.volume = 0.64;
    defaultSource.muted = false;
    defaultSource.available = true;
    defaultSource.isDefault = true;
    defaultSource.isVirtual = false;
    defaultSource.backendApi = QStringLiteral("alsa");

    AudioSourceState secondarySource;
    secondarySource.id = QStringLiteral("bluez_input.headset.1");
    secondarySource.label = QStringLiteral("Headset Microphone");
    secondarySource.volume = 0.28;
    secondarySource.muted = true;
    secondarySource.available = true;
    secondarySource.isDefault = false;
    secondarySource.isVirtual = false;
    secondarySource.backendApi = QStringLiteral("bluez");

    state.sinks = {defaultSink, secondarySink};
    state.selectedSinkId = defaultSink.id;
    state.sources = {defaultSource, secondarySource};
    state.selectedSourceId = defaultSource.id;
    return state;
}

AudioState alternateAudioState()
{
    AudioState state = sampleAudioState();
    state.selectedSinkId = state.sinks.at(1).id;
    state.sinks[0].isDefault = false;
    state.sinks[0].volume = 0.42;
    state.sinks[1].isDefault = true;
    state.sinks[1].muted = false;
    state.sinks[1].volume = 0.9;
    state.selectedSourceId = state.sources.at(1).id;
    state.sources[0].isDefault = false;
    state.sources[0].volume = 0.47;
    state.sources[1].isDefault = true;
    state.sources[1].muted = false;
    state.sources[1].volume = 0.52;
    return state;
}

WindowSnapshot sampleWindowSnapshot()
{
    WindowSnapshot snapshot;

    WindowState editorWindow;
    editorWindow.id = QStringLiteral("window-editor");
    editorWindow.title = QStringLiteral("CHANGELOG.md - Kate");
    editorWindow.appId = QStringLiteral("org.kde.kate");
    editorWindow.pid = 4201;
    editorWindow.isActive = true;
    editorWindow.isMinimized = false;
    editorWindow.isMaximized = true;
    editorWindow.isFullscreen = false;
    editorWindow.isOnAllDesktops = false;
    editorWindow.skipTaskbar = false;
    editorWindow.skipSwitcher = false;
    editorWindow.geometry = {40, 30, 1440, 960};
    editorWindow.clientGeometry = {40, 58, 1440, 932};
    editorWindow.virtualDesktopIds = {QStringLiteral("desktop-1")};
    editorWindow.activityIds = {QStringLiteral("activity-work")};
    editorWindow.resourceName = QStringLiteral("kate");

    WindowState terminalWindow;
    terminalWindow.id = QStringLiteral("window-terminal");
    terminalWindow.title = QStringLiteral("plasma-bridge - Konsole");
    terminalWindow.appId = QStringLiteral("org.kde.konsole");
    terminalWindow.pid = 4310;
    terminalWindow.isActive = false;
    terminalWindow.isMinimized = false;
    terminalWindow.isMaximized = false;
    terminalWindow.isFullscreen = false;
    terminalWindow.isOnAllDesktops = false;
    terminalWindow.skipTaskbar = false;
    terminalWindow.skipSwitcher = false;
    terminalWindow.geometry = {96, 120, 960, 640};
    terminalWindow.clientGeometry = {96, 148, 960, 612};
    terminalWindow.virtualDesktopIds = {QStringLiteral("desktop-1")};
    terminalWindow.parentId = editorWindow.id;

    snapshot.windows = {terminalWindow, editorWindow};
    snapshot.activeWindowId = editorWindow.id;
    return snapshot;
}

WindowSnapshot sampleWindowSnapshotWithoutActiveWindow()
{
    WindowSnapshot snapshot = sampleWindowSnapshot();
    snapshot.activeWindowId.clear();
    for (WindowState &window : snapshot.windows) {
        window.isActive = false;
    }
    return snapshot;
}

void ensureDocsResourcesInitialized()
{
    initDocsResources();
}

QJsonObject parseJsonObject(const QByteArray &json)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    Q_ASSERT(parseError.error == QJsonParseError::NoError);
    Q_ASSERT(document.isObject());
    return document.object();
}

QUrl httpUrl(const quint16 port, const QString &path)
{
    return QUrl(QStringLiteral("http://%1:%2%3").arg(QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST), QString::number(port), path));
}

QUrl wsUrl(const quint16 port, const QString &path)
{
    return QUrl(QStringLiteral("ws://%1:%2%3").arg(QStringLiteral(PLASMA_BRIDGE_DEFAULT_HOST), QString::number(port), path));
}

FakeAudioProbeSource::FakeAudioProbeSource(QObject *parent)
    : AudioProbeSource(parent)
{
}

void FakeAudioProbeSource::start()
{
    ++m_startCount;
}

const AudioState &FakeAudioProbeSource::currentState() const
{
    return m_state;
}

bool FakeAudioProbeSource::hasInitialState() const
{
    return m_ready;
}

void FakeAudioProbeSource::setState(const AudioState &state, const bool ready)
{
    m_state = state;
    m_ready = ready;
}

void FakeAudioProbeSource::emitInitialStateReady(const AudioState &state)
{
    setState(state, true);
    emit initialStateReady();
}

void FakeAudioProbeSource::emitAudioStateChanged(const QString &reason,
                                                 const QString &sinkId,
                                                 const QString &sourceId,
                                                 const AudioState &state)
{
    m_state = state;
    emit audioStateChanged(reason, sinkId, sourceId);
}

void FakeAudioProbeSource::emitConnectionFailure(const QString &message)
{
    emit connectionFailed(message);
}

int FakeAudioProbeSource::startCount() const
{
    return m_startCount;
}

FakeWindowProbeSource::FakeWindowProbeSource(QObject *parent)
    : WindowProbeSource(parent)
{
}

void FakeWindowProbeSource::start()
{
    ++m_startCount;
}

const WindowSnapshot &FakeWindowProbeSource::currentSnapshot() const
{
    return m_snapshot;
}

bool FakeWindowProbeSource::hasInitialSnapshot() const
{
    return m_ready;
}

QString FakeWindowProbeSource::backendName() const
{
    return QStringLiteral("kwin-script-helper");
}

void FakeWindowProbeSource::setSnapshot(const WindowSnapshot &snapshot, const bool ready)
{
    m_snapshot = snapshot;
    m_ready = ready;
}

void FakeWindowProbeSource::emitInitialSnapshotReady(const WindowSnapshot &snapshot)
{
    setSnapshot(snapshot, true);
    emit initialSnapshotReady();
}

void FakeWindowProbeSource::emitConnectionFailure(const QString &message)
{
    emit connectionFailed(message);
}

int FakeWindowProbeSource::startCount() const
{
    return m_startCount;
}

FakeWindowProbeBackendController::FakeWindowProbeBackendController(QObject *parent)
    : WindowProbeBackendController(parent)
{
    const tools::window_probe::WindowProbeBackendStatus defaultStatus{
        QStringLiteral("kwin-script-helper"), true, true, true, false, true, true};
    m_setupResult = {true, QStringLiteral("Synthetic setup success."), defaultStatus};
    m_statusResult = {true, QStringLiteral("Synthetic status success."), defaultStatus};
    m_teardownResult = {true,
                        QStringLiteral("Synthetic teardown success."),
                        tools::window_probe::WindowProbeBackendStatus{QStringLiteral("kwin-script-helper"),
                                                                      false,
                                                                      false,
                                                                      false,
                                                                      false,
                                                                      false,
                                                                      false}};
    m_activationResult.status = control::WindowActivationStatus::Accepted;
}

tools::window_probe::WindowProbeCommandResult FakeWindowProbeBackendController::setup()
{
    return m_setupResult;
}

tools::window_probe::WindowProbeCommandResult FakeWindowProbeBackendController::status()
{
    return m_statusResult;
}

tools::window_probe::WindowProbeCommandResult FakeWindowProbeBackendController::teardown()
{
    return m_teardownResult;
}

control::WindowActivationResult FakeWindowProbeBackendController::activateWindow(const QString &windowId)
{
    m_lastActivationWindowId = windowId;
    ++m_activationCallCount;

    control::WindowActivationResult result = m_activationResult;
    if (result.windowId.isEmpty()) {
        result.windowId = windowId;
    }
    return result;
}

void FakeWindowProbeBackendController::setSetupResult(const tools::window_probe::WindowProbeCommandResult &result)
{
    m_setupResult = result;
}

void FakeWindowProbeBackendController::setStatusResult(const tools::window_probe::WindowProbeCommandResult &result)
{
    m_statusResult = result;
}

void FakeWindowProbeBackendController::setTeardownResult(const tools::window_probe::WindowProbeCommandResult &result)
{
    m_teardownResult = result;
}

void FakeWindowProbeBackendController::setActivationResult(const control::WindowActivationResult &result)
{
    m_activationResult = result;
}

QString FakeWindowProbeBackendController::lastActivationWindowId() const
{
    return m_lastActivationWindowId;
}

int FakeWindowProbeBackendController::activationCallCount() const
{
    return m_activationCallCount;
}

FakeSubmissionGate::FakeSubmissionGate(QObject *parent)
    : AudioControlSubmissionGate(parent)
{
}

bool FakeSubmissionGate::shouldSubmitRequest() const
{
    return m_shouldSubmitRequest;
}

void FakeSubmissionGate::setShouldSubmitRequest(const bool value)
{
    if (m_shouldSubmitRequest == value) {
        return;
    }

    m_shouldSubmitRequest = value;
    emit submissionConditionChanged();
}

void FakeAudioVolumeController::setResult(const Operation operation, const control::VolumeChangeResult &result)
{
    switch (operation) {
    case Operation::Set:
        m_setResult = result;
        break;
    case Operation::Increment:
        m_incrementResult = result;
        break;
    case Operation::Decrement:
        m_decrementResult = result;
        break;
    case Operation::None:
        break;
    }
}

control::VolumeChangeResult FakeAudioVolumeController::setVolume(const QString &sinkId, const double value)
{
    return invoke(Operation::Set, sinkId, value);
}

control::VolumeChangeResult FakeAudioVolumeController::incrementVolume(const QString &sinkId, const double value)
{
    return invoke(Operation::Increment, sinkId, value);
}

control::VolumeChangeResult FakeAudioVolumeController::decrementVolume(const QString &sinkId, const double value)
{
    return invoke(Operation::Decrement, sinkId, value);
}

FakeAudioVolumeController::Operation FakeAudioVolumeController::lastOperation() const
{
    return m_lastOperation;
}

QString FakeAudioVolumeController::lastSinkId() const
{
    return m_lastSinkId;
}

double FakeAudioVolumeController::lastValue() const
{
    return m_lastValue;
}

int FakeAudioVolumeController::callCount() const
{
    return m_callCount;
}

control::VolumeChangeResult FakeAudioVolumeController::invoke(const Operation operation, const QString &sinkId, const double value)
{
    m_lastOperation = operation;
    m_lastSinkId = sinkId;
    m_lastValue = value;
    ++m_callCount;

    switch (operation) {
    case Operation::Set:
        return m_setResult;
    case Operation::Increment:
        return m_incrementResult;
    case Operation::Decrement:
        return m_decrementResult;
    case Operation::None:
        break;
    }

    return {};
}

void FakeAudioDeviceController::setDefaultResult(const Operation operation, const control::DefaultDeviceChangeResult &result)
{
    switch (operation) {
    case Operation::SetDefaultSink:
        m_setDefaultSinkResult = result;
        break;
    case Operation::SetDefaultSource:
        m_setDefaultSourceResult = result;
        break;
    case Operation::None:
    case Operation::SetSinkMute:
    case Operation::SetSourceMute:
        break;
    }
}

void FakeAudioDeviceController::setMuteResult(const Operation operation, const control::MuteChangeResult &result)
{
    switch (operation) {
    case Operation::SetSinkMute:
        m_setSinkMuteResult = result;
        break;
    case Operation::SetSourceMute:
        m_setSourceMuteResult = result;
        break;
    case Operation::None:
    case Operation::SetDefaultSink:
    case Operation::SetDefaultSource:
        break;
    }
}

control::DefaultDeviceChangeResult FakeAudioDeviceController::setDefaultSink(const QString &sinkId)
{
    return invokeDefault(Operation::SetDefaultSink, sinkId);
}

control::DefaultDeviceChangeResult FakeAudioDeviceController::setDefaultSource(const QString &sourceId)
{
    return invokeDefault(Operation::SetDefaultSource, sourceId);
}

control::MuteChangeResult FakeAudioDeviceController::setSinkMuted(const QString &sinkId, const bool muted)
{
    return invokeMute(Operation::SetSinkMute, sinkId, muted);
}

control::MuteChangeResult FakeAudioDeviceController::setSourceMuted(const QString &sourceId, const bool muted)
{
    return invokeMute(Operation::SetSourceMute, sourceId, muted);
}

FakeAudioDeviceController::Operation FakeAudioDeviceController::lastOperation() const
{
    return m_lastOperation;
}

QString FakeAudioDeviceController::lastDeviceId() const
{
    return m_lastDeviceId;
}

std::optional<bool> FakeAudioDeviceController::lastMuted() const
{
    return m_lastMuted;
}

int FakeAudioDeviceController::callCount() const
{
    return m_callCount;
}

control::DefaultDeviceChangeResult FakeAudioDeviceController::invokeDefault(const Operation operation, const QString &deviceId)
{
    m_lastOperation = operation;
    m_lastDeviceId = deviceId;
    m_lastMuted.reset();
    ++m_callCount;

    switch (operation) {
    case Operation::SetDefaultSink:
        return m_setDefaultSinkResult;
    case Operation::SetDefaultSource:
        return m_setDefaultSourceResult;
    case Operation::None:
    case Operation::SetSinkMute:
    case Operation::SetSourceMute:
        break;
    }

    return {};
}

control::MuteChangeResult FakeAudioDeviceController::invokeMute(const Operation operation, const QString &deviceId, const bool muted)
{
    m_lastOperation = operation;
    m_lastDeviceId = deviceId;
    m_lastMuted = muted;
    ++m_callCount;

    switch (operation) {
    case Operation::SetSinkMute:
        return m_setSinkMuteResult;
    case Operation::SetSourceMute:
        return m_setSourceMuteResult;
    case Operation::None:
    case Operation::SetDefaultSink:
    case Operation::SetDefaultSource:
        break;
    }

    return {};
}

void FakeWindowActivationController::setResult(const control::WindowActivationResult &result)
{
    m_result = result;
}

control::WindowActivationResult FakeWindowActivationController::activateWindow(const QString &windowId)
{
    m_lastWindowId = windowId;
    ++m_callCount;

    control::WindowActivationResult result = m_result;
    if (result.windowId.isEmpty()) {
        result.windowId = windowId;
    }
    return result;
}

QString FakeWindowActivationController::lastWindowId() const
{
    return m_lastWindowId;
}

int FakeWindowActivationController::callCount() const
{
    return m_callCount;
}

} // namespace plasma_bridge::tests
