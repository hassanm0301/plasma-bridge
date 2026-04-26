#include "adapters/audio/pulse_audio_state_observer.h"

#include <PulseAudioQt/Context>
#include <PulseAudioQt/Device>
#include <PulseAudioQt/PulseObject>
#include <PulseAudioQt/Server>
#include <PulseAudioQt/Sink>
#include <PulseAudioQt/Source>
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

bool deviceAvailable(const PulseAudioQt::Device *device)
{
    return device != nullptr && device->state() != DeviceState::InvalidState;
}

bool deviceVirtual(const PulseAudioQt::Device *device)
{
    if (device == nullptr) {
        return false;
    }

    if (device->isVirtualDevice()) {
        return true;
    }

    const QVariantMap properties = device->pulseProperties();
    return properties.value(QStringLiteral("node.virtual")).toBool();
}

bool isMonitorSource(const PulseAudioQt::Source *source)
{
    if (source == nullptr) {
        return false;
    }

    if (source->name().endsWith(QStringLiteral(".monitor"), Qt::CaseInsensitive)) {
        return true;
    }

    const QVariantMap properties = source->pulseProperties();
    if (properties.contains(QStringLiteral("monitor_of_sink"))
        || properties.contains(QStringLiteral("monitor.source"))) {
        return true;
    }

    const QString deviceClass = properties.value(QStringLiteral("device.class")).toString();
    if (deviceClass.contains(QStringLiteral("monitor"), Qt::CaseInsensitive)) {
        return true;
    }

    const QString mediaClass = properties.value(QStringLiteral("media.class")).toString();
    return mediaClass.contains(QStringLiteral("monitor"), Qt::CaseInsensitive);
}

double normalizedVolume(const PulseAudioQt::VolumeObject *volumeObject)
{
    if (volumeObject == nullptr) {
        return 0.0;
    }

    return static_cast<double>(volumeObject->volume()) / static_cast<double>(PulseAudioQt::normalVolume());
}

plasma_bridge::AudioDeviceState deviceStateFromObject(const PulseAudioQt::Device *device, const QString &selectedId)
{
    const QVariantMap properties = device == nullptr ? QVariantMap() : device->pulseProperties();

    plasma_bridge::AudioDeviceState state;
    if (device == nullptr) {
        return state;
    }

    state.id = device->name();
    state.label = device->description().isEmpty() ? device->name() : device->description();
    state.volume = normalizedVolume(device);
    state.muted = device->isMuted();
    state.available = deviceAvailable(device);
    state.isDefault = (!selectedId.isEmpty() && state.id == selectedId)
        || (selectedId.isEmpty() && device->isDefault());
    state.isVirtual = deviceVirtual(device);
    state.backendApi = properties.value(QStringLiteral("device.api")).toString();
    return state;
}

void sortDevices(QList<plasma_bridge::AudioDeviceState> *devices)
{
    if (devices == nullptr) {
        return;
    }

    std::sort(devices->begin(), devices->end(), [](const plasma_bridge::AudioDeviceState &left,
                                                   const plasma_bridge::AudioDeviceState &right) {
        const int labelCompare = QString::compare(left.label, right.label, Qt::CaseInsensitive);
        if (labelCompare != 0) {
            return labelCompare < 0;
        }

        return QString::compare(left.id, right.id, Qt::CaseInsensitive) < 0;
    });
}

} // namespace

PulseAudioStateObserver::PulseAudioStateObserver(QObject *parent)
    : QObject(parent)
{
    m_initialPublishTimer.setSingleShot(true);
    connect(&m_initialPublishTimer, &QTimer::timeout, this, [this]() {
        if (m_initialStatePublished) {
            return;
        }

        if (m_context != nullptr) {
            for (PulseAudioQt::Sink *sink : m_context->sinks()) {
                attachSink(sink);
            }

            for (PulseAudioQt::Source *source : m_context->sources()) {
                attachSource(source);
            }
        }

        m_state = buildState();
        m_initialStatePublished = true;
        emit initialStateReady();
    });
}

void PulseAudioStateObserver::start()
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

    connect(m_context, &PulseAudioQt::Context::stateChanged, this, &PulseAudioStateObserver::handleContextStateChanged);
    connect(m_context, &PulseAudioQt::Context::sinkAdded, this, [this](PulseAudioQt::Sink *sink) {
        attachSink(sink);
        if (!m_initialStatePublished) {
            scheduleInitialPublish();
            return;
        }

        refreshAndEmit(QStringLiteral("sink-added"), sink ? sink->name() : QString(), QString());
    });
    connect(m_context, &PulseAudioQt::Context::sinkRemoved, this, [this](PulseAudioQt::Sink *sink) {
        if (!m_initialStatePublished) {
            scheduleInitialPublish();
            return;
        }

        refreshAndEmit(QStringLiteral("sink-removed"), sink ? sink->name() : QString(), QString());
    });
    connect(m_context, &PulseAudioQt::Context::sourceAdded, this, [this](PulseAudioQt::Source *source) {
        if (isMonitorSource(source)) {
            return;
        }

        attachSource(source);
        if (!m_initialStatePublished) {
            scheduleInitialPublish();
            return;
        }

        refreshAndEmit(QStringLiteral("source-added"), QString(), source ? source->name() : QString());
    });
    connect(m_context, &PulseAudioQt::Context::sourceRemoved, this, [this](PulseAudioQt::Source *source) {
        if (isMonitorSource(source)) {
            return;
        }

        if (!m_initialStatePublished) {
            scheduleInitialPublish();
            return;
        }

        refreshAndEmit(QStringLiteral("source-removed"), QString(), source ? source->name() : QString());
    });

    if (m_server != nullptr) {
        connect(m_server, &PulseAudioQt::Server::defaultSinkChanged, this, [this](PulseAudioQt::Sink *sink) {
            if (!m_initialStatePublished) {
                scheduleInitialPublish();
                return;
            }

            refreshAndEmit(QStringLiteral("default-sink-changed"), sink ? sink->name() : QString(), QString());
        });
        connect(m_server, &PulseAudioQt::Server::defaultSourceChanged, this, [this](PulseAudioQt::Source *source) {
            if (isMonitorSource(source)) {
                return;
            }

            if (!m_initialStatePublished) {
                scheduleInitialPublish();
                return;
            }

            refreshAndEmit(QStringLiteral("default-source-changed"), QString(), source ? source->name() : QString());
        });
    }

    handleContextStateChanged();
}

const plasma_bridge::AudioState &PulseAudioStateObserver::currentState() const
{
    return m_state;
}

bool PulseAudioStateObserver::hasInitialState() const
{
    return m_initialStatePublished;
}

bool PulseAudioStateObserver::isPipeWireServer() const
{
    return m_server != nullptr && m_server->isPipeWire();
}

QString PulseAudioStateObserver::serverVersion() const
{
    return m_server == nullptr ? QString() : m_server->version();
}

void PulseAudioStateObserver::attachSink(PulseAudioQt::Sink *sink)
{
    if (sink == nullptr || m_attachedDevices.contains(sink)) {
        return;
    }

    m_attachedDevices.insert(sink);

    connect(sink, &QObject::destroyed, this, [this, sink]() {
        m_attachedDevices.remove(sink);
    });

    const auto emitSinkUpdated = [this, sink]() {
        if (m_initialStatePublished) {
            refreshAndEmit(QStringLiteral("sink-updated"), sink ? sink->name() : QString(), QString());
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

void PulseAudioStateObserver::attachSource(PulseAudioQt::Source *source)
{
    if (source == nullptr || isMonitorSource(source) || m_attachedDevices.contains(source)) {
        return;
    }

    m_attachedDevices.insert(source);

    connect(source, &QObject::destroyed, this, [this, source]() {
        m_attachedDevices.remove(source);
    });

    const auto emitSourceUpdated = [this, source]() {
        if (m_initialStatePublished && !isMonitorSource(source)) {
            refreshAndEmit(QStringLiteral("source-updated"), QString(), source ? source->name() : QString());
        }
    };

    connect(source, &PulseAudioQt::PulseObject::nameChanged, this, emitSourceUpdated);
    connect(source, &PulseAudioQt::VolumeObject::volumeChanged, this, emitSourceUpdated);
    connect(source, &PulseAudioQt::VolumeObject::mutedChanged, this, emitSourceUpdated);
    connect(source, &PulseAudioQt::Device::descriptionChanged, this, emitSourceUpdated);
    connect(source, &PulseAudioQt::Device::stateChanged, this, emitSourceUpdated);
    connect(source, &PulseAudioQt::Device::defaultChanged, this, emitSourceUpdated);
    connect(source, &PulseAudioQt::Device::pulsePropertiesChanged, this, emitSourceUpdated);
    connect(source, &PulseAudioQt::Device::virtualDeviceChanged, this, emitSourceUpdated);
}

void PulseAudioStateObserver::scheduleInitialPublish()
{
    if (m_initialStatePublished) {
        return;
    }

    m_initialPublishTimer.start(200);
}

void PulseAudioStateObserver::refreshAndEmit(const QString &reason, const QString &sinkId, const QString &sourceId)
{
    m_state = buildState();
    emit audioStateChanged(reason, sinkId, sourceId);
}

plasma_bridge::AudioState PulseAudioStateObserver::buildState() const
{
    plasma_bridge::AudioState state;

    if (m_context == nullptr) {
        return state;
    }

    const PulseAudioQt::Sink *defaultSink = m_server == nullptr ? nullptr : m_server->defaultSink();
    const PulseAudioQt::Source *defaultSource = m_server == nullptr ? nullptr : m_server->defaultSource();
    state.selectedSinkId = defaultSink == nullptr ? QString() : defaultSink->name();
    state.selectedSourceId = defaultSource == nullptr || isMonitorSource(defaultSource) ? QString() : defaultSource->name();

    for (PulseAudioQt::Sink *sink : m_context->sinks()) {
        if (sink == nullptr) {
            continue;
        }

        state.sinks.append(deviceStateFromObject(sink, state.selectedSinkId));
    }

    for (PulseAudioQt::Source *source : m_context->sources()) {
        if (source == nullptr || isMonitorSource(source)) {
            continue;
        }

        state.sources.append(deviceStateFromObject(source, state.selectedSourceId));
    }

    sortDevices(&state.sinks);
    sortDevices(&state.sources);
    return state;
}

void PulseAudioStateObserver::handleContextStateChanged()
{
    if (m_context == nullptr) {
        return;
    }

    const ContextState state = m_context->state();
    if (state == ContextState::Ready) {
        for (PulseAudioQt::Sink *sink : m_context->sinks()) {
            attachSink(sink);
        }

        for (PulseAudioQt::Source *source : m_context->sources()) {
            attachSource(source);
        }

        if (!m_initialStatePublished) {
            scheduleInitialPublish();
            return;
        }

        refreshAndEmit(QStringLiteral("context-ready"), QString(), QString());
        return;
    }

    if (!m_initialStatePublished && (state == ContextState::Failed || state == ContextState::Terminated)) {
        emit connectionFailed(QStringLiteral("PulseAudioQt context entered state: %1").arg(contextStateName(state)));
    }
}

} // namespace plasma_bridge::audio
