#include "control/audio_volume_controller.h"

#include <QtTest>

#include <limits>

class AudioVolumeControllerFormattingTest : public QObject
{
    Q_OBJECT

private slots:
    void statusName_data();
    void statusName();
    void jsonFormattingHandlesNullAndFiniteValues();
    void humanReadableFormattingShowsValueStates();
};

void AudioVolumeControllerFormattingTest::statusName_data()
{
    QTest::addColumn<int>("status");
    QTest::addColumn<QString>("expected");

    QTest::newRow("accepted") << static_cast<int>(plasma_bridge::control::VolumeChangeStatus::Accepted) << QStringLiteral("accepted");
    QTest::newRow("not-ready") << static_cast<int>(plasma_bridge::control::VolumeChangeStatus::NotReady) << QStringLiteral("not_ready");
    QTest::newRow("missing") << static_cast<int>(plasma_bridge::control::VolumeChangeStatus::SinkNotFound) << QStringLiteral("sink_not_found");
    QTest::newRow("not-writable") << static_cast<int>(plasma_bridge::control::VolumeChangeStatus::SinkNotWritable) << QStringLiteral("sink_not_writable");
    QTest::newRow("invalid") << static_cast<int>(plasma_bridge::control::VolumeChangeStatus::InvalidValue) << QStringLiteral("invalid_value");
}

void AudioVolumeControllerFormattingTest::statusName()
{
    QFETCH(int, status);
    QFETCH(QString, expected);

    QCOMPARE(plasma_bridge::control::volumeChangeStatusName(static_cast<plasma_bridge::control::VolumeChangeStatus>(status)),
             expected);
}

void AudioVolumeControllerFormattingTest::jsonFormattingHandlesNullAndFiniteValues()
{
    plasma_bridge::control::VolumeChangeResult result;
    result.status = plasma_bridge::control::VolumeChangeStatus::Accepted;
    result.sinkId = QStringLiteral("sink-1");
    result.requestedValue = std::numeric_limits<double>::quiet_NaN();
    result.targetValue = 0.5;

    const QJsonObject json = plasma_bridge::control::toJsonObject(result);

    QCOMPARE(json.value(QStringLiteral("status")).toString(), QStringLiteral("accepted"));
    QCOMPARE(json.value(QStringLiteral("sinkId")).toString(), QStringLiteral("sink-1"));
    QVERIFY(json.value(QStringLiteral("requestedValue")).isNull());
    QCOMPARE(json.value(QStringLiteral("targetValue")).toDouble(), 0.5);
    QVERIFY(json.value(QStringLiteral("previousValue")).isNull());
}

void AudioVolumeControllerFormattingTest::humanReadableFormattingShowsValueStates()
{
    plasma_bridge::control::VolumeChangeResult result;
    result.status = plasma_bridge::control::VolumeChangeStatus::InvalidValue;
    result.sinkId = QString();
    result.requestedValue = 0.25;
    result.targetValue = std::numeric_limits<double>::infinity();

    const QString text = plasma_bridge::control::formatHumanReadableResult(result);

    QVERIFY(text.contains(QStringLiteral("Status: invalid_value")));
    QVERIFY(text.contains(QStringLiteral("Sink: (none)")));
    QVERIFY(text.contains(QStringLiteral("Requested Value: 0.2500 (25.0%)")));
    QVERIFY(text.contains(QStringLiteral("Target Value: (invalid)")));
    QVERIFY(text.contains(QStringLiteral("Previous Value: (none)")));
}

QTEST_GUILESS_MAIN(AudioVolumeControllerFormattingTest)

#include "test_control.moc"
