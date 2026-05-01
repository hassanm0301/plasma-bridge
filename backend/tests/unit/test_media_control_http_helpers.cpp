#include "api/media_control_http_helpers.h"

#include <QtTest>

class MediaControlHttpHelpersTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesRoutes();
    void mapsStatusesToHttpCodes();
};

void MediaControlHttpHelpersTest::parsesRoutes()
{
    using namespace plasma_bridge::api;

    QCOMPARE(parseMediaControlRoute(QStringLiteral("/control/media/current/play")).match, MediaControlRouteMatch::Match);
    QCOMPARE(parseMediaControlRoute(QStringLiteral("/control/media/current/pause")).route.action,
             plasma_bridge::control::MediaControlAction::Pause);
    QCOMPARE(parseMediaControlRoute(QStringLiteral("/control/media/current/play-pause")).route.action,
             plasma_bridge::control::MediaControlAction::PlayPause);
    QCOMPARE(parseMediaControlRoute(QStringLiteral("/control/media/current/next")).route.action,
             plasma_bridge::control::MediaControlAction::Next);
    QCOMPARE(parseMediaControlRoute(QStringLiteral("/control/media/current/previous")).route.action,
             plasma_bridge::control::MediaControlAction::Previous);
    QCOMPARE(parseMediaControlRoute(QStringLiteral("/control/media/current/seek")).route.action,
             plasma_bridge::control::MediaControlAction::Seek);
}

void MediaControlHttpHelpersTest::mapsStatusesToHttpCodes()
{
    QCOMPARE(plasma_bridge::api::httpStatusCodeForMediaControlStatus(
                 plasma_bridge::control::MediaControlStatus::Accepted),
             200);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForMediaControlStatus(
                 plasma_bridge::control::MediaControlStatus::NoCurrentPlayer),
             404);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForMediaControlStatus(
                 plasma_bridge::control::MediaControlStatus::ActionNotSupported),
             409);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForMediaControlStatus(
                 plasma_bridge::control::MediaControlStatus::NotReady),
             503);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForMediaControlStatus(
                 plasma_bridge::control::MediaControlStatus::PlayerUnavailable),
             503);
}

QTEST_GUILESS_MAIN(MediaControlHttpHelpersTest)

#include "test_media_control_http_helpers.moc"
