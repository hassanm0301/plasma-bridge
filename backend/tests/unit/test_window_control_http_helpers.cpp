#include "api/window_control_http_helpers.h"

#include <QtTest>

class WindowControlHttpHelpersTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesRoutesAndRejectsInvalidIds();
    void mapsActivationStatusesToHttpCodes();
    void mapsCloseStatusesToHttpCodes();
};

void WindowControlHttpHelpersTest::parsesRoutesAndRejectsInvalidIds()
{
    using namespace plasma_bridge::api;

    const WindowControlRouteParseResult activate =
        parseWindowControlRoute(QStringLiteral("/control/windows/window-editor/active"));
    QCOMPARE(activate.match, WindowControlRouteMatch::Match);
    QCOMPARE(activate.route.windowId, QStringLiteral("window-editor"));
    QCOMPARE(activate.route.action, WindowControlAction::Activate);

    const WindowControlRouteParseResult encoded =
        parseWindowControlRoute(QStringLiteral("/control/windows/window%20with%20space/active"));
    QCOMPARE(encoded.match, WindowControlRouteMatch::Match);
    QCOMPARE(encoded.route.windowId, QStringLiteral("window with space"));
    QCOMPARE(encoded.route.action, WindowControlAction::Activate);

    const WindowControlRouteParseResult close =
        parseWindowControlRoute(QStringLiteral("/control/windows/window-editor/close"));
    QCOMPARE(close.match, WindowControlRouteMatch::Match);
    QCOMPARE(close.route.windowId, QStringLiteral("window-editor"));
    QCOMPARE(close.route.action, WindowControlAction::Close);

    const WindowControlRouteParseResult invalid =
        parseWindowControlRoute(QStringLiteral("/control/windows/window%2Fpart/active"));
    QCOMPARE(invalid.match, WindowControlRouteMatch::InvalidWindowId);

    const WindowControlRouteParseResult noMatch =
        parseWindowControlRoute(QStringLiteral("/control/windows/window-editor/focus"));
    QCOMPARE(noMatch.match, WindowControlRouteMatch::NoMatch);
}

void WindowControlHttpHelpersTest::mapsActivationStatusesToHttpCodes()
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

void WindowControlHttpHelpersTest::mapsCloseStatusesToHttpCodes()
{
    QCOMPARE(plasma_bridge::api::httpStatusCodeForWindowCloseStatus(
                 plasma_bridge::control::WindowCloseStatus::Accepted),
             200);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForWindowCloseStatus(
                 plasma_bridge::control::WindowCloseStatus::WindowNotFound),
             404);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForWindowCloseStatus(
                 plasma_bridge::control::WindowCloseStatus::WindowNotCloseable),
             409);
    QCOMPARE(plasma_bridge::api::httpStatusCodeForWindowCloseStatus(
                 plasma_bridge::control::WindowCloseStatus::NotReady),
             503);
}

QTEST_GUILESS_MAIN(WindowControlHttpHelpersTest)

#include "test_window_control_http_helpers.moc"
