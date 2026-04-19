#include "tests/support/test_support.h"

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
    return QUrl(QStringLiteral("http://127.0.0.1:%1%2").arg(QString::number(port), path));
}

QUrl wsUrl(const quint16 port, const QString &path)
{
    return QUrl(QStringLiteral("ws://127.0.0.1:%1%2").arg(QString::number(port), path));
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

} // namespace plasma_bridge::tests
