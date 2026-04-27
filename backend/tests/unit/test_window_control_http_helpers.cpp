#include "api/window_control_http_helpers.h"

#include <QtTest>

class WindowControlHttpHelpersTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesRoutesAndRejectsInvalidIds();
    void mapsStatusesToHttpCodes();
};

void WindowControlHttpHelpersTest::parsesRoutesAndRejectsInvalidIds()
{
    using namespace plasma_bridge::api;

    const WindowControlRouteParseResult activate =
        parseWindowControlRoute(QStringLiteral("/control/windows/window-editor/active"));
    QCOMPARE(activate.match, WindowControlRouteMatch::Match);
    QCOMPARE(activate.route.windowId, QStringLiteral("window-editor"));

    const WindowControlRouteParseResult encoded =
        parseWindowControlRoute(QStringLiteral("/control/windows/window%20with%20space/active"));
    QCOMPARE(encoded.match, WindowControlRouteMatch::Match);
    QCOMPARE(encoded.route.windowId, QStringLiteral("window with space"));

    const WindowControlRouteParseResult invalid =
        parseWindowControlRoute(QStringLiteral("/control/windows/window%2Fpart/active"));
    QCOMPARE(invalid.match, WindowControlRouteMatch::InvalidWindowId);

    const WindowControlRouteParseResult noMatch =
        parseWindowControlRoute(QStringLiteral("/control/windows/window-editor/focus"));
    QCOMPARE(noMatch.match, WindowControlRouteMatch::NoMatch);
}

void WindowControlHttpHelpersTest::mapsStatusesToHttpCodes()
{
    QCOMPARE(plasma_bridge::api::httpStatusCodeForWindowActivationStatus(
                 plasma_bridge::control::WindowActivationStatus::Accepted),
             200);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForWindowActivationStatus(
                 plasma_bridge::control::WindowActivationStatus::WindowNotFound),
             404);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForWindowActivationStatus(
                 plasma_bridge::control::WindowActivationStatus::WindowNotActivatable),
             409);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForWindowActivationStatus(
                 plasma_bridge::control::WindowActivationStatus::NotReady),
             503);
}

QTEST_GUILESS_MAIN(WindowControlHttpHelpersTest)

#include "test_window_control_http_helpers.moc"
