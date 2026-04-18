#include "adapters/audio/pulse_audio_sink_observer.h"

#include <PulseAudioQt/Context>
#include <PulseAudioQt/Device>
#include <PulseAudioQt/PulseObject>
#include <PulseAudioQt/Server>
#include <PulseAudioQt/Sink>
#include <PulseAudioQt/VolumeObject>

#include <QVariantMap>

#include <algorithm>

namespace plasma_bridge::audio
{
namespace
{

using ContextState = PulseAudioQt::Context::State;
using DeviceState = PulseAudioQt::Device::State;

QString contextStateName(ContextState state)
{
    switch (state) {
    case ContextState::Unconnected:
        return QStringLiteral("unconnected");
    case ContextState::Connecting:
        return QStringLiteral("connecting");
    case ContextState::Authorizing:
        return QStringLiteral("authorizing");
    case ContextState::SettingName:
        return QStringLiteral("setting-name");
    case ContextState::Ready:
        return QStringLiteral("ready");
    case ContextState::Failed:
        return QStringLiteral("failed");
    case ContextState::Terminated:
        return QStringLiteral("terminated");
    }

    return QStringLiteral("unknown");
}

bool sinkAvailable(PulseAudioQt::Sink *sink)
{
    const DeviceState state = sink->state();
    return state != DeviceState::InvalidState;
}

bool sinkVirtual(PulseAudioQt::Sink *sink)
{
    if (sink->isVirtualDevice()) {
        return true;
    }

    const QVariantMap properties = sink->pulseProperties();
    return properties.value(QStringLiteral("node.virtual")).toBool();
}

double normalizedVolume(PulseAudioQt::Sink *sink)
{
    return static_cast<double>(sink->volume()) / static_cast<double>(PulseAudioQt::normalVolume());
}

plasma_bridge::AudioSinkState sinkStateFromObject(PulseAudioQt::Sink *sink, const QString &selectedSinkId)
{
    const QVariantMap properties = sink->pulseProperties();

    plasma_bridge::AudioSinkState state;
    state.id = sink->name();
    state.label = sink->description().isEmpty() ? sink->name() : sink->description();
    state.volume = normalizedVolume(sink);
    state.muted = sink->isMuted();
    state.available = sinkAvailable(sink);
    state.isDefault = !selectedSinkId.isEmpty() && state.id == selectedSinkId;
    state.isVirtual = sinkVirtual(sink);
    state.backendApi = properties.value(QStringLiteral("device.api")).toString();
    return state;
}

} // namespace

PulseAudioSinkObserver::PulseAudioSinkObserver(QObject *parent)
    : QObject(parent)
{
    m_initialPublishTimer.setSingleShot(true);
    connect(&m_initialPublishTimer, &QTimer::timeout, this, [this]() {
        if (m_initialStatePublished) {
            return;
        }

        for (PulseAudioQt::Sink *sink : m_context == nullptr ? QList<PulseAudioQt::Sink *>() : m_context->sinks()) {
            attachSink(sink);
        }

        m_state = buildState();
        m_initialStatePublished = true;
        emit initialStateReady();
    });
}

void PulseAudioSinkObserver::start()
{
    if (m_started) {
        return;
    }
    m_started = true;

    m_context = PulseAudioQt::Context::instance();
    if (m_context == nullptr) {
        emit connectionFailed(QStringLiteral("PulseAudioQt context is unavailable."));
        return;
    }

    m_server = m_context->server();

    connect(m_context, &PulseAudioQt::Context::stateChanged, this, &PulseAudioSinkObserver::handleContextStateChanged);
    connect(m_context, &PulseAudioQt::Context::sinkAdded, this, [this](PulseAudioQt::Sink *sink) {
        attachSink(sink);
        if (!m_initialStatePublished) {
            scheduleInitialPublish();
            return;
        }

        refreshAndEmit(QStringLiteral("sink-added"), sink ? sink->name() : QString());
    });
    connect(m_context, &PulseAudioQt::Context::sinkRemoved, this, [this](PulseAudioQt::Sink *sink) {
        if (!m_initialStatePublished) {
            scheduleInitialPublish();
            return;
        }

        refreshAndEmit(QStringLiteral("sink-removed"), sink ? sink->name() : QString());
    });

    if (m_server != nullptr) {
        connect(m_server, &PulseAudioQt::Server::defaultSinkChanged, this, [this](PulseAudioQt::Sink *sink) {
            if (!m_initialStatePublished) {
                scheduleInitialPublish();
                return;
            }

            refreshAndEmit(QStringLiteral("default-sink-changed"), sink ? sink->name() : QString());
        });
    }

    handleContextStateChanged();
}

const plasma_bridge::AudioState &PulseAudioSinkObserver::currentState() const
{
    return m_state;
}

bool PulseAudioSinkObserver::hasInitialState() const
{
    return m_initialStatePublished;
}

bool PulseAudioSinkObserver::isPipeWireServer() const
{
    return m_server != nullptr && m_server->isPipeWire();
}

QString PulseAudioSinkObserver::serverVersion() const
{
    return m_server == nullptr ? QString() : m_server->version();
}

void PulseAudioSinkObserver::attachSink(PulseAudioQt::Sink *sink)
{
    if (sink == nullptr || m_attachedSinks.contains(sink)) {
        return;
    }

    m_attachedSinks.insert(sink);

    connect(sink, &QObject::destroyed, this, [this, sink]() {
        m_attachedSinks.remove(sink);
    });

    const auto emitSinkUpdated = [this, sink]() {
        if (m_initialStatePublished) {
            refreshAndEmit(QStringLiteral("sink-updated"), sink ? sink->name() : QString());
        }
    };

    connect(sink, &PulseAudioQt::PulseObject::nameChanged, this, emitSinkUpdated);
    connect(sink, &PulseAudioQt::VolumeObject::volumeChanged, this, emitSinkUpdated);
    connect(sink, &PulseAudioQt::VolumeObject::mutedChanged, this, emitSinkUpdated);
    connect(sink, &PulseAudioQt::Device::descriptionChanged, this, emitSinkUpdated);
    connect(sink, &PulseAudioQt::Device::stateChanged, this, emitSinkUpdated);
    connect(sink, &PulseAudioQt::Device::defaultChanged, this, emitSinkUpdated);
    connect(sink, &PulseAudioQt::Device::pulsePropertiesChanged, this, emitSinkUpdated);
    connect(sink, &PulseAudioQt::Device::virtualDeviceChanged, this, emitSinkUpdated);
}

void PulseAudioSinkObserver::scheduleInitialPublish()
{
    if (m_initialStatePublished) {
        return;
    }

    m_initialPublishTimer.start(200);
}

void PulseAudioSinkObserver::refreshAndEmit(const QString &reason, const QString &sinkId)
{
    m_state = buildState();
    emit audioStateChanged(reason, sinkId);
}

plasma_bridge::AudioState PulseAudioSinkObserver::buildState() const
{
    plasma_bridge::AudioState state;

    if (m_context == nullptr) {
        return state;
    }

    const PulseAudioQt::Sink *defaultSink = m_server == nullptr ? nullptr : m_server->defaultSink();
    state.selectedSinkId = defaultSink == nullptr ? QString() : defaultSink->name();

    for (PulseAudioQt::Sink *sink : m_context->sinks()) {
        if (sink == nullptr) {
            continue;
        }

        state.sinks.append(sinkStateFromObject(sink, state.selectedSinkId));
    }

    std::sort(state.sinks.begin(), state.sinks.end(), [](const plasma_bridge::AudioSinkState &left,
                                                         const plasma_bridge::AudioSinkState &right) {
        const int labelCompare = QString::compare(left.label, right.label, Qt::CaseInsensitive);
        if (labelCompare != 0) {
            return labelCompare < 0;
        }

        return QString::compare(left.id, right.id, Qt::CaseInsensitive) < 0;
    });

    return state;
}

void PulseAudioSinkObserver::handleContextStateChanged()
{
    if (m_context == nullptr) {
        return;
    }

    const ContextState state = m_context->state();
    if (state == ContextState::Ready) {
        for (PulseAudioQt::Sink *sink : m_context->sinks()) {
            attachSink(sink);
        }

        if (!m_initialStatePublished) {
            scheduleInitialPublish();
            return;
        }

        refreshAndEmit(QStringLiteral("context-ready"), QString());
        return;
    }

    if (!m_initialStatePublished && (state == ContextState::Failed || state == ContextState::Terminated)) {
        emit connectionFailed(QStringLiteral("PulseAudioQt context entered state: %1").arg(contextStateName(state)));
    }
}

} // namespace plasma_bridge::audio
