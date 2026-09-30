#include "ApplicationSettings.h"

#include <QTemporaryDir>
#include <QtTest>

class ThemeSettingsTest : public QObject
{
    Q_OBJECT
    QTemporaryDir directory;

private slots:
    void initTestCase()
    {
        QVERIFY(directory.isValid());
        QCoreApplication::setOrganizationName(QStringLiteral("NotepadNextTests"));
        QCoreApplication::setApplicationName(QStringLiteral("ThemeSettingsTest"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, directory.path());
    }

    void init()
    {
        QSettings settings;
        settings.clear();
        settings.sync();
    }

    void newInstallationFollowsSystem()
    {
        ApplicationSettings settings;
        QCOMPARE(settings.themeMode(), ApplicationSettings::FollowSystem);
        QVERIFY(!settings.contains("Gui/DarkMode"));
    }

    void migrateExplicitPreference_data()
    {
        QTest::addColumn<bool>("dark");
        QTest::newRow("old-light") << false;
        QTest::newRow("old-dark") << true;
    }

    void migrateExplicitPreference()
    {
        QFETCH(bool, dark);
        QSettings().setValue("Gui/DarkMode", dark);
        ApplicationSettings settings;
        QCOMPARE(settings.themeMode(), dark ? ApplicationSettings::DarkTheme : ApplicationSettings::LightTheme);
        QVERIFY(settings.contains("Gui/ThemeMode"));
    }

    void newPreferenceWinsOverLegacyValue()
    {
        QSettings legacy;
        legacy.setValue("Gui/DarkMode", true);
        legacy.setValue("Gui/ThemeMode", int(ApplicationSettings::FollowSystem));
        ApplicationSettings settings;
        QCOMPARE(settings.themeMode(), ApplicationSettings::FollowSystem);
    }

    void choicePersistsAcrossSettingsInstances()
    {
        ApplicationSettings settings;
        QSignalSpy changed(&settings, &ApplicationSettings::themeModeChanged);
        for (auto mode : {ApplicationSettings::DarkTheme, ApplicationSettings::LightTheme, ApplicationSettings::FollowSystem}) {
            settings.setThemeMode(mode);
            settings.sync();
            ApplicationSettings reopened;
            QCOMPARE(reopened.themeMode(), mode);
        }
        QCOMPARE(changed.count(), 3);
        settings.setThemeMode(ApplicationSettings::FollowSystem);
        QCOMPARE(changed.count(), 3);
    }

    void invalidStoredValueFallsBackToSystem()
    {
        ApplicationSettings settings;
        settings.setValue("Gui/ThemeMode", QStringLiteral("invalid"));
        QCOMPARE(settings.themeMode(), ApplicationSettings::FollowSystem);
        settings.setValue("Gui/ThemeMode", 99);
        QCOMPARE(settings.themeMode(), ApplicationSettings::FollowSystem);
    }

    void resetReturnsToSystemDefault()
    {
        QSettings().setValue("Gui/DarkMode", true);
        ApplicationSettings settings;
        settings.clear();
        QCOMPARE(settings.themeMode(), ApplicationSettings::FollowSystem);
    }
};

QTEST_GUILESS_MAIN(ThemeSettingsTest)
#include "ThemeSettingsTest.moc"
