#include "control/favorite_apps_store.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class FavoriteAppsStoreTest : public QObject
{
    Q_OBJECT

private slots:
    void loadsMissingFileAsEmpty();
    void savesAndLoadsNormalizedIds();
    void rejectsMalformedJson();
    void reportsSaveFailureWhenParentIsFile();
};

void FavoriteAppsStoreTest::loadsMissingFileAsEmpty()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    plasma_bridge::control::FavoriteAppsStore store(tempDir.filePath(QStringLiteral("favorite_apps.json")));
    const plasma_bridge::control::FavoriteAppsStoreLoadResult result = store.load();

    QCOMPARE(result.status, plasma_bridge::control::FavoriteAppsStoreStatus::Accepted);
    QVERIFY(result.appIds.isEmpty());
}

void FavoriteAppsStoreTest::savesAndLoadsNormalizedIds()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    plasma_bridge::control::FavoriteAppsStore store(tempDir.filePath(QStringLiteral("favorite_apps.json")));
    const plasma_bridge::control::FavoriteAppsStoreSaveResult saveResult = store.save(
        {QStringLiteral("org.kde.kate.desktop"),
         QStringLiteral("org.kde.konsole.desktop"),
         QStringLiteral("org.kde.kate.desktop"),
         QString()});
    QCOMPARE(saveResult.status, plasma_bridge::control::FavoriteAppsStoreStatus::Accepted);

    const plasma_bridge::control::FavoriteAppsStoreLoadResult loadResult = store.load();
    QCOMPARE(loadResult.status, plasma_bridge::control::FavoriteAppsStoreStatus::Accepted);
    QCOMPARE(loadResult.appIds,
             QStringList({QStringLiteral("org.kde.kate.desktop"), QStringLiteral("org.kde.konsole.desktop")}));
}

void FavoriteAppsStoreTest::rejectsMalformedJson()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = tempDir.filePath(QStringLiteral("favorite_apps.json"));
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("{not-json");
    file.close();

    plasma_bridge::control::FavoriteAppsStore store(filePath);
    const plasma_bridge::control::FavoriteAppsStoreLoadResult result = store.load();

    QCOMPARE(result.status, plasma_bridge::control::FavoriteAppsStoreStatus::StorageError);
    QVERIFY(!result.errorMessage.isEmpty());
}

void FavoriteAppsStoreTest::reportsSaveFailureWhenParentIsFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString parentFilePath = tempDir.filePath(QStringLiteral("favorites-root"));
    QFile parentFile(parentFilePath);
    QVERIFY(parentFile.open(QIODevice::WriteOnly));
    parentFile.write("x");
    parentFile.close();

    plasma_bridge::control::FavoriteAppsStore store(
        tempDir.filePath(QStringLiteral("favorites-root/favorite_apps.json")));
    const plasma_bridge::control::FavoriteAppsStoreSaveResult result =
        store.save({QStringLiteral("org.kde.kate.desktop")});

    QCOMPARE(result.status, plasma_bridge::control::FavoriteAppsStoreStatus::StorageError);
    QVERIFY(!result.errorMessage.isEmpty());
}

QTEST_GUILESS_MAIN(FavoriteAppsStoreTest)

#include "test_favorite_apps_store.moc"
