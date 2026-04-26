#include "common/window_state.h"
#include "tests/support/test_support.h"

#include <QJsonArray>
#include <QtTest>

class WindowStateTest : public QObject
{
    Q_OBJECT

private slots:
    void geometryJsonUsesCoordinates();
    void windowJsonUsesNullForUnsetOptionalFields();
    void snapshotJsonUsesNullForMissingActiveWindow();
    void snapshotRoundTripsThroughJsonHelpers();
    void activeWindowFallsBackToActiveFlag();
    void normalizeSnapshotMakesActiveStateConsistent();
    void humanReadableFormattingIncludesImportantFields();
};

void WindowStateTest::geometryJsonUsesCoordinates()
{
    const plasma_bridge::WindowGeometryState geometry{12, 24, 1280, 720};
    const QJsonObject json = plasma_bridge::toJsonObject(geometry);

    QCOMPARE(json.value(QStringLiteral("x")).toInt(), geometry.x);
    QCOMPARE(json.value(QStringLiteral("y")).toInt(), geometry.y);
    QCOMPARE(json.value(QStringLiteral("width")).toInt(), geometry.width);
    QCOMPARE(json.value(QStringLiteral("height")).toInt(), geometry.height);
    QCOMPARE(plasma_bridge::toQRect(geometry), QRect(12, 24, 1280, 720));
}

void WindowStateTest::windowJsonUsesNullForUnsetOptionalFields()
{
    plasma_bridge::WindowState window;
    window.id = QStringLiteral("window-1");
    window.title = QStringLiteral("Example");
    window.geometry = {1, 2, 3, 4};
    window.clientGeometry = {5, 6, 7, 8};

    const QJsonObject json = plasma_bridge::toJsonObject(window);

    QCOMPARE(json.value(QStringLiteral("id")).toString(), window.id);
    QCOMPARE(json.value(QStringLiteral("title")).toString(), window.title);
    QVERIFY(json.value(QStringLiteral("appId")).isNull());
    QVERIFY(json.value(QStringLiteral("pid")).isNull());
    QVERIFY(json.value(QStringLiteral("parentId")).isNull());
    QVERIFY(json.value(QStringLiteral("resourceName")).isNull());
    QCOMPARE(json.value(QStringLiteral("geometry")).toObject().value(QStringLiteral("width")).toInt(), 3);
    QCOMPARE(json.value(QStringLiteral("clientGeometry")).toObject().value(QStringLiteral("height")).toInt(), 8);
    QCOMPARE(json.value(QStringLiteral("virtualDesktopIds")).toArray().size(), 0);
    QCOMPARE(json.value(QStringLiteral("activityIds")).toArray().size(), 0);
}

void WindowStateTest::snapshotJsonUsesNullForMissingActiveWindow()
{
    const plasma_bridge::WindowSnapshot snapshot = plasma_bridge::tests::sampleWindowSnapshotWithoutActiveWindow();
    const QJsonObject json = plasma_bridge::toJsonObject(snapshot);

    QCOMPARE(json.value(QStringLiteral("windows")).toArray().size(), snapshot.windows.size());
    QVERIFY(json.value(QStringLiteral("activeWindowId")).isNull());
}

void WindowStateTest::snapshotRoundTripsThroughJsonHelpers()
{
    const plasma_bridge::WindowSnapshot snapshot = plasma_bridge::tests::sampleWindowSnapshot();
    const QJsonObject json = plasma_bridge::toJsonObject(snapshot);

    const std::optional<plasma_bridge::WindowSnapshot> parsed = plasma_bridge::windowSnapshotFromJson(json);

    QVERIFY(parsed.has_value());
    QCOMPARE(parsed->activeWindowId, QStringLiteral("window-editor"));
    QCOMPARE(parsed->windows.size(), 2);
    QCOMPARE(parsed->windows.at(0).id, QStringLiteral("window-terminal"));
    QCOMPARE(parsed->windows.at(1).resourceName, QStringLiteral("kate"));
}

void WindowStateTest::activeWindowFallsBackToActiveFlag()
{
    plasma_bridge::WindowSnapshot snapshot = plasma_bridge::tests::sampleWindowSnapshot();
    snapshot.activeWindowId.clear();

    const std::optional<plasma_bridge::WindowState> activeWindow = plasma_bridge::activeWindow(snapshot);

    QVERIFY(activeWindow.has_value());
    QCOMPARE(activeWindow->id, QStringLiteral("window-editor"));
}

void WindowStateTest::normalizeSnapshotMakesActiveStateConsistent()
{
    plasma_bridge::WindowSnapshot snapshot = plasma_bridge::tests::sampleWindowSnapshot();
    snapshot.activeWindowId = QStringLiteral("window-terminal");
    snapshot.windows[0].isActive = false;
    snapshot.windows[1].isActive = true;

    const plasma_bridge::WindowSnapshot normalized = plasma_bridge::normalizeWindowSnapshot(snapshot);
    const QJsonObject json = plasma_bridge::toJsonObject(snapshot);

    QCOMPARE(normalized.activeWindowId, QStringLiteral("window-terminal"));
    QCOMPARE(normalized.windows[0].id, QStringLiteral("window-terminal"));
    QCOMPARE(normalized.windows[0].isActive, true);
    QCOMPARE(normalized.windows[1].isActive, false);
    QCOMPARE(json.value(QStringLiteral("activeWindowId")).toString(), QStringLiteral("window-terminal"));
    QCOMPARE(json.value(QStringLiteral("activeWindow")).toObject().value(QStringLiteral("id")).toString(),
             QStringLiteral("window-terminal"));
    QCOMPARE(json.value(QStringLiteral("windows")).toArray().at(0).toObject().value(QStringLiteral("isActive")).toBool(),
             true);
}

void WindowStateTest::humanReadableFormattingIncludesImportantFields()
{
    const plasma_bridge::WindowSnapshot snapshot = plasma_bridge::tests::sampleWindowSnapshot();
    const QString listText = plasma_bridge::formatHumanReadableWindowList(snapshot);
    const QString activeText = plasma_bridge::formatHumanReadableActiveWindow(plasma_bridge::activeWindow(snapshot));

    QVERIFY(listText.contains(QStringLiteral("Window Count: 2")));
    QVERIFY(listText.contains(QStringLiteral("Active Window: window-editor")));
    QVERIFY(listText.contains(QStringLiteral("* CHANGELOG.md - Kate")));
    QVERIFY(listText.contains(QStringLiteral("resourceName: kate")));

    QVERIFY(activeText.contains(QStringLiteral("Active Window:")));
    QVERIFY(activeText.contains(QStringLiteral("id: window-editor")));
    QVERIFY(activeText.contains(QStringLiteral("appId: org.kde.kate")));
}

QTEST_GUILESS_MAIN(WindowStateTest)

#include "test_window_state.moc"
