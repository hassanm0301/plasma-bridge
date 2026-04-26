#include "tools/probes/audio_probe/audio_probe_runner.h"

#include "adapters/audio/pulse_audio_state_observer.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QTextStream>
#include <QTimer>

namespace plasma_bridge::tools::audio_probe
{

class PulseAudioProbeSource::Impl final : public QObject
{
    Q_OBJECT

public:
    explicit Impl(PulseAudioProbeSource *owner)
        : QObject(owner)
    {
        connect(&m_observer, &audio::PulseAudioStateObserver::initialStateReady, owner, &AudioProbeSource::initialStateReady);
        connect(&m_observer, &audio::PulseAudioStateObserver::audioStateChanged, owner, &AudioProbeSource::audioStateChanged);
        connect(&m_observer, &audio::PulseAudioStateObserver::connectionFailed, owner, &AudioProbeSource::connectionFailed);
    }

    audio::PulseAudioStateObserver m_observer;
};

PulseAudioProbeSource::PulseAudioProbeSource(QObject *parent)
    : AudioProbeSource(parent)
    , m_impl(new Impl(this))
{
}

void PulseAudioProbeSource::start()
{
    m_impl->m_observer.start();
}

const plasma_bridge::AudioState &PulseAudioProbeSource::currentState() const
{
    return m_impl->m_observer.currentState();
}

bool PulseAudioProbeSource::hasInitialState() const
{
    return m_impl->m_observer.hasInitialState();
}

AudioProbeRunner::AudioProbeRunner(AudioProbeSource *source,
                                   const AudioProbeOptions &options,
                                   QTextStream *output,
                                   QTextStream *error,
                                   const int startupTimeoutMs,
                                   QObject *parent)
    : QObject(parent)
    , m_source(source)
    , m_options(options)
    , m_output(output)
    , m_error(error)
    , m_startupTimeoutMs(startupTimeoutMs)
    , m_startupTimer(new QTimer(this))
{
    m_startupTimer->setSingleShot(true);

    connect(m_startupTimer, &QTimer::timeout, this, [this]() {
        if (m_source != nullptr && !m_source->hasInitialState()) {
            if (m_error != nullptr) {
                *m_error << "Timed out waiting for PulseAudioQt audio state." << Qt::endl;
            }
            finish(1);
        }
    });

    if (m_source == nullptr) {
        return;
    }

    connect(m_source, &AudioProbeSource::connectionFailed, this, [this](const QString &message) {
        if (m_error != nullptr) {
            *m_error << message << Qt::endl;
        }
        if (m_source != nullptr && !m_source->hasInitialState()) {
            finish(1);
        }
    });

    connect(m_source, &AudioProbeSource::initialStateReady, this, [this]() {
        publishInitialState();
    });

    connect(m_source,
            &AudioProbeSource::audioStateChanged,
            this,
            [this](const QString &reason, const QString &sinkId, const QString &sourceId) {
        if (!m_options.watchMode || m_source == nullptr) {
            return;
        }

        printEvent(reason, sinkId, sourceId, m_options.jsonOutput);
    });
}

void AudioProbeRunner::start()
{
    if (m_finished || m_source == nullptr) {
        if (!m_finished) {
            finish(1);
        }
        return;
    }

    if (m_startupTimeoutMs >= 0) {
        m_startupTimer->start(m_startupTimeoutMs);
    }

    m_source->start();

    if (m_source->hasInitialState()) {
        publishInitialState();
    }
}

void AudioProbeRunner::finish(const int exitCode)
{
    if (m_finished) {
        return;
    }

    m_finished = true;
    m_startupTimer->stop();
    emit finished(exitCode);
}

void AudioProbeRunner::publishInitialState()
{
    if (m_initialStatePublished || m_source == nullptr || !m_source->hasInitialState()) {
        return;
    }

    m_initialStatePublished = true;
    m_startupTimer->stop();
    printEvent(QStringLiteral("initial"), QString(), QString(), m_options.watchMode);

    if (!m_options.watchMode) {
        finish(0);
    }
}

void AudioProbeRunner::printEvent(const QString &reason,
                                  const QString &sinkId,
                                  const QString &sourceId,
                                  const bool compactOutput) const
{
    if (m_output == nullptr || m_source == nullptr) {
        return;
    }

    if (m_options.jsonOutput) {
        *m_output << formatJsonEventBytes(reason, sinkId, sourceId, m_source->currentState(), compactOutput) << Qt::endl;
        return;
    }

    *m_output << formatHumanEventText(reason, sinkId, sourceId, m_source->currentState()) << Qt::endl;
}

void configureParser(QCommandLineParser &parser)
{
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption(QStringLiteral("watch"),
                                        QStringLiteral("Keep running and print live audio updates.")));
    parser.addOption(QCommandLineOption(QStringLiteral("json"),
                                        QStringLiteral("Print machine-readable JSON output.")));
}

AudioProbeOptions optionsFromParser(const QCommandLineParser &parser)
{
    AudioProbeOptions options;
    options.watchMode = parser.isSet(QStringLiteral("watch"));
    options.jsonOutput = parser.isSet(QStringLiteral("json"));
    return options;
}

QByteArray formatJsonEventBytes(const QString &reason,
                                const QString &sinkId,
                                const QString &sourceId,
                                const plasma_bridge::AudioState &state,
                                const bool compactOutput)
{
    return QJsonDocument(plasma_bridge::toJsonEventObject(reason, sinkId, sourceId, state))
        .toJson(compactOutput ? QJsonDocument::Compact : QJsonDocument::Indented);
}

QString formatHumanEventText(const QString &reason,
                             const QString &sinkId,
                             const QString &sourceId,
                             const plasma_bridge::AudioState &state)
{
    return plasma_bridge::formatHumanReadableEvent(reason, sinkId, sourceId, state);
}

} // namespace plasma_bridge::tools::audio_probe

#include "audio_probe_runner.moc"
