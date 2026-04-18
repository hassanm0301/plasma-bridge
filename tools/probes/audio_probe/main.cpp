#include "adapters/audio/pulse_audio_sink_observer.h"
#include "common/audio_state.h"

#include <PulseAudioQt/Context>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QTextStream>
#include <QTimer>

namespace
{

constexpr int kStartupTimeoutMs = 5000;

void printJsonEvent(const QString &reason,
                    const QString &sinkId,
                    const plasma_bridge::AudioState &state,
                    bool compactOutput)
{
    const QJsonDocument document(plasma_bridge::toJsonEventObject(reason, sinkId, state));
    QTextStream output(stdout);
    output << document.toJson(compactOutput ? QJsonDocument::Compact : QJsonDocument::Indented) << Qt::endl;
}

void printHumanEvent(const QString &reason, const QString &sinkId, const plasma_bridge::AudioState &state)
{
    QTextStream output(stdout);
    output << plasma_bridge::formatHumanReadableEvent(reason, sinkId, state) << Qt::endl;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("audio_probe"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Observe KDE-visible audio sinks through KF6PulseAudioQt."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption watchOption(QStringLiteral("watch"),
                                   QStringLiteral("Keep running and print live sink updates."));
    QCommandLineOption jsonOption(QStringLiteral("json"),
                                  QStringLiteral("Print machine-readable JSON output."));

    parser.addOption(watchOption);
    parser.addOption(jsonOption);
    parser.process(app);

    const bool watchMode = parser.isSet(watchOption);
    const bool jsonOutput = parser.isSet(jsonOption);

    PulseAudioQt::Context::setApplicationId(QStringLiteral("org.plasma-remote-toolbar.audio_probe"));

    plasma_bridge::audio::PulseAudioSinkObserver observer;

    QTimer startupTimer;
    startupTimer.setSingleShot(true);

    QObject::connect(&startupTimer, &QTimer::timeout, &app, [&]() {
        if (!observer.hasInitialState()) {
            QTextStream error(stderr);
            error << "Timed out waiting for PulseAudioQt sink state." << Qt::endl;
            app.exit(1);
        }
    });

    QObject::connect(&observer, &plasma_bridge::audio::PulseAudioSinkObserver::connectionFailed, &app, [&](const QString &message) {
        QTextStream error(stderr);
        error << message << Qt::endl;
        if (!observer.hasInitialState()) {
            app.exit(1);
        }
    });

    QObject::connect(&observer, &plasma_bridge::audio::PulseAudioSinkObserver::initialStateReady, &app, [&]() {
        startupTimer.stop();

        const QString initialReason = QStringLiteral("initial");
        if (jsonOutput) {
            printJsonEvent(initialReason, QString(), observer.currentState(), watchMode);
        } else {
            printHumanEvent(initialReason, QString(), observer.currentState());
        }

        if (!watchMode) {
            app.quit();
        }
    });

    QObject::connect(&observer, &plasma_bridge::audio::PulseAudioSinkObserver::audioStateChanged, &app, [&](const QString &reason,
                                                                                                               const QString &sinkId) {
        if (!watchMode) {
            return;
        }

        if (jsonOutput) {
            printJsonEvent(reason, sinkId, observer.currentState(), true);
        } else {
            printHumanEvent(reason, sinkId, observer.currentState());
        }
    });

    startupTimer.start(kStartupTimeoutMs);
    observer.start();

    return app.exec();
}
