#include "api/app_control_http_helpers.h"

#include <QtTest>

class AppControlHttpHelpersTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesRoutesAndRejectsInvalidIds();
    void parsesAppOpenOptions();
    void mapsFavoriteStatusesToHttpCodes();
    void mapsOpenStatusesToHttpCodes();
};

void AppControlHttpHelpersTest::parsesRoutesAndRejectsInvalidIds()
{
    using namespace plasma_bridge::api;

    const AppControlRouteParseResult favorite =
        parseAppControlRoute(QStringLiteral("/control/apps/org.kde.kate.desktop/favorite"));
    QCOMPARE(favorite.match, AppControlRouteMatch::Match);
    QCOMPARE(favorite.route.appId, QStringLiteral("org.kde.kate.desktop"));
    QCOMPARE(favorite.route.action, AppControlAction::Favorite);

    const AppControlRouteParseResult unfavorite =
        parseAppControlRoute(QStringLiteral("/control/apps/org.kde.konsole.desktop/unfavorite"));
    QCOMPARE(unfavorite.match, AppControlRouteMatch::Match);
    QCOMPARE(unfavorite.route.appId, QStringLiteral("org.kde.konsole.desktop"));
    QCOMPARE(unfavorite.route.action, AppControlAction::Unfavorite);

    const AppControlRouteParseResult open =
        parseAppControlRoute(QStringLiteral("/control/apps/org.kde.kate.desktop/open"));
    QCOMPARE(open.match, AppControlRouteMatch::Match);
    QCOMPARE(open.route.appId, QStringLiteral("org.kde.kate.desktop"));
    QCOMPARE(open.route.action, AppControlAction::Open);

    const AppControlRouteParseResult encoded =
        parseAppControlRoute(QStringLiteral("/control/apps/My%20App.desktop/open"));
    QCOMPARE(encoded.match, AppControlRouteMatch::Match);
    QCOMPARE(encoded.route.appId, QStringLiteral("My App.desktop"));

    const AppControlRouteParseResult invalid =
        parseAppControlRoute(QStringLiteral("/control/apps/org.kde%2Fkate.desktop/open"));
    QCOMPARE(invalid.match, AppControlRouteMatch::InvalidAppId);

    const AppControlRouteParseResult noMatch =
        parseAppControlRoute(QStringLiteral("/control/apps/org.kde.kate.desktop/launch"));
    QCOMPARE(noMatch.match, AppControlRouteMatch::NoMatch);
}

void AppControlHttpHelpersTest::mapsFavoriteStatusesToHttpCodes()
{
    QCOMPARE(plasma_bridge::api::httpStatusCodeForAppFavoriteChangeStatus(
                 plasma_bridge::control::AppFavoriteChangeStatus::Accepted),
             200);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForAppFavoriteChangeStatus(
                 plasma_bridge::control::AppFavoriteChangeStatus::AppNotFound),
             404);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForAppFavoriteChangeStatus(
                 plasma_bridge::control::AppFavoriteChangeStatus::StorageError),
             500);
}

void AppControlHttpHelpersTest::parsesAppOpenOptions()
{
    plasma_bridge::control::AppOpenOptions options;
    QString errorMessage;

    QVERIFY(plasma_bridge::api::parseAppOpenOptions(QByteArrayLiteral("/control/apps/org.kde.kate.desktop/open"),
                                                    &options,
                                                    &errorMessage));
    QCOMPARE(options.switchToExisting, false);
    QVERIFY(errorMessage.isEmpty());

    QVERIFY(plasma_bridge::api::parseAppOpenOptions(
        QByteArrayLiteral("/control/apps/org.kde.kate.desktop/open?switchToExisting=true"),
        &options,
        &errorMessage));
    QCOMPARE(options.switchToExisting, true);

    QVERIFY(plasma_bridge::api::parseAppOpenOptions(
        QByteArrayLiteral("/control/apps/org.kde.kate.desktop/open?switchToExisting=false"),
        &options,
        &errorMessage));
    QCOMPARE(options.switchToExisting, false);

    QVERIFY(!plasma_bridge::api::parseAppOpenOptions(
        QByteArrayLiteral("/control/apps/org.kde.kate.desktop/open?switchToExisting=maybe"),
        &options,
        &errorMessage));
    QCOMPARE(errorMessage, QStringLiteral("switchToExisting must be true or false."));
}

void AppControlHttpHelpersTest::mapsOpenStatusesToHttpCodes()
{
    QCOMPARE(plasma_bridge::api::httpStatusCodeForAppOpenStatus(
                 plasma_bridge::control::AppOpenStatus::Accepted),
             200);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForAppOpenStatus(
                 plasma_bridge::control::AppOpenStatus::AppNotFound),
             404);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForAppOpenStatus(
                 plasma_bridge::control::AppOpenStatus::LaunchFailed),
             500);
}

QTEST_GUILESS_MAIN(AppControlHttpHelpersTest)

#include "test_app_control_http_helpers.moc"
