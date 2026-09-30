/*
 * This file is part of Notepad Next.
 * Copyright 2024 Justin Dailey
 *
 * Notepad Next is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Notepad Next is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Notepad Next.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ApplicationSettings.h"

#include <QApplication>
#include <QFont>

#define CREATE_SETTING(group, name, lname, type, default) \
ApplicationSetting<type> name{#group "/" #name, default};\
    type ApplicationSettings::lname() const\
{\
        return get(name);\
}\
    void ApplicationSettings::set##name(type lname)\
{\
        set(name, lname);\
        emit lname##Changed(lname);\
}



ApplicationSettings::ApplicationSettings(QObject *parent)
    : QSettings{parent}
{
    // Only migrate an explicitly saved choice. A fresh installation follows
    // the system; the old implicit false default was not a user preference.
    if (!contains(QStringLiteral("Gui/ThemeMode")) && contains(QStringLiteral("Gui/DarkMode"))) {
        setValue(QStringLiteral("Gui/ThemeMode"),
                 static_cast<int>(value(QStringLiteral("Gui/DarkMode")).toBool() ? DarkTheme : LightTheme));
    }
}

ApplicationSettings::ThemeMode ApplicationSettings::themeMode() const
{
    bool ok = false;
    const int mode = value(QStringLiteral("Gui/ThemeMode"), static_cast<int>(FollowSystem)).toInt(&ok);
    return ok && mode >= FollowSystem && mode <= DarkTheme ? static_cast<ThemeMode>(mode) : FollowSystem;
}

void ApplicationSettings::setThemeMode(ThemeMode mode)
{
    if (mode < FollowSystem || mode > DarkTheme || mode == themeMode())
        return;
    setValue(QStringLiteral("Gui/ThemeMode"), static_cast<int>(mode));
    emit themeModeChanged(mode);
}

CREATE_SETTING(Gui, ShowMenuBar, showMenuBar, bool, true)
CREATE_SETTING(Gui, ShowToolBar, showToolBar, bool, true)
CREATE_SETTING(Gui, ShowTabBar, showTabBar, bool, true)
CREATE_SETTING(Gui, ShowStatusBar, showStatusBar, bool, true)
CREATE_SETTING(Gui, CenterSearchDialog, centerSearchDialog, bool, true)

CREATE_SETTING(Gui, TabsClosable, tabsClosable, bool, true)
CREATE_SETTING(Gui, ExitOnLastTabClosed, exitOnLastTabClosed, bool, false)

CREATE_SETTING(Gui, CombineSearchResults, combineSearchResults, bool, false)

CREATE_SETTING(App, RestorePreviousSession, restorePreviousSession, bool, false)
CREATE_SETTING(App, RestoreUnsavedFiles, restoreUnsavedFiles, bool, false)
CREATE_SETTING(App, RestoreTempFiles, restoreTempFiles, bool, false)

CREATE_SETTING(App, DefaultDirectoryBehavior, defaultDirectoryBehavior, ApplicationSettings::DefaultDirectoryBehaviorEnum, ApplicationSettings::FollowCurrentDocument)
CREATE_SETTING(App, DefaultDirectory, defaultDirectory, QString, QString())

CREATE_SETTING(App, Translation, translation, QString, QStringLiteral(""))

CREATE_SETTING(Editor, ShowWhitespace, showWhitespace, bool, false);
CREATE_SETTING(Editor, ShowEndOfLine, showEndOfLine, bool, false);
CREATE_SETTING(Editor, ShowWrapSymbol, showWrapSymbol, bool, false);
CREATE_SETTING(Editor, ShowIndentGuide, showIndentGuide, bool, true);
CREATE_SETTING(Editor, WordWrap, wordWrap, bool, false)
CREATE_SETTING(Editor, FontName, fontName, QString, QStringLiteral("Courier New"))
CREATE_SETTING(Editor, FontSize, fontSize, int, []() { return qApp->font().pointSize() + 2; })
CREATE_SETTING(Editor, AdditionalWordChars, additionalWordChars, QString, QStringLiteral(""));
CREATE_SETTING(Editor, DefaultEOLMode, defaultEOLMode, QString, QStringLiteral(""))
CREATE_SETTING(Editor, URLHighlighting, urlHighlighting, bool, true)
CREATE_SETTING(Editor, ShowLineNumbers, showLineNumbers, bool, true)
CREATE_SETTING(Editor, AutoCompletion, autoCompletion, bool, true)
