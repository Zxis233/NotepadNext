#include "Theme.h"
#include "SciLexer.h"

#include <QPalette>
#include <QRegularExpression>
#include <QtTest>

class ThemePaletteTest : public QObject
{
    Q_OBJECT
private slots:
    void scintillaByteOrder()
    {
        QCOMPARE(Theme::scintillaColour(0x0d1117), 0x17110d);
        QCOMPARE(Theme::scintillaColour(0x264f78), 0x784f26);
        QCOMPARE(Theme::scintillaElementColour(0xf0f6fc), 0xfffcf6f0u);
        QCOMPARE(Theme::scintillaElementColour(0x264f78), 0xff784f26u);
    }

    void githubInterfacePalette()
    {
        const QPalette palette = Theme::applicationPalette(true);
        for (auto group : {QPalette::Active, QPalette::Inactive}) {
            QCOMPARE(palette.color(group, QPalette::Base), QColor("#0d1117"));
            QCOMPARE(palette.color(group, QPalette::Window), QColor("#151b23"));
            QCOMPARE(palette.color(group, QPalette::Text), QColor("#f0f6fc"));
            QCOMPARE(palette.color(group, QPalette::Highlight), QColor("#264f78"));
            QCOMPARE(palette.color(group, QPalette::Link), QColor("#58a6ff"));
        }
        QCOMPARE(palette.color(QPalette::Disabled, QPalette::Text), QColor("#656c76"));
    }

    void lightPaletteIsPreserved()
    {
        const QPalette palette = Theme::applicationPalette(false);
        QCOMPARE(palette.color(QPalette::Window), QColor("#f0f0f0"));
        QCOMPARE(palette.color(QPalette::Base), QColor("#ffffff"));
        QCOMPARE(palette.color(QPalette::Text), QColor("#000000"));
        QCOMPARE(palette.color(QPalette::Button), QColor("#f0f0f0"));
        QCOMPARE(palette.color(QPalette::Highlight), QColor("#308cc6"));
        QCOMPARE(palette.color(QPalette::ToolTipBase), QColor("#ffffdc"));
        QCOMPARE(palette.color(QPalette::Disabled, QPalette::Text), QColor("#808080"));
        QVERIFY(Theme::widgetStyleSheet(false).isEmpty());
    }

    void semanticSyntaxColors()
    {
        QCOMPARE(Theme::darkStyleForeground("cpp", SCE_C_COMMENT, "SCE_C_COMMENT", "comment", 0x008000), Theme::Dark::Muted);
        QCOMPARE(Theme::darkStyleForeground("cpp", SCE_C_PREPROCESSORCOMMENT, "SCE_C_PREPROCESSORCOMMENT", "comment preprocessor", 0x008000), Theme::Dark::Muted);
        QCOMPARE(Theme::darkStyleForeground("cpp", SCE_C_WORD2, "SCE_C_WORD2", "identifier", 0), Theme::Dark::Keyword);
        QCOMPARE(Theme::darkStyleForeground("cpp", SCE_C_IDENTIFIER, "SCE_C_IDENTIFIER", "identifier", 0), Theme::Dark::Foreground);
        QCOMPARE(Theme::darkStyleForeground("cpp", SCE_C_STRING, "SCE_C_STRING", "literal string", 0x808080), Theme::Dark::String);
        QCOMPARE(Theme::darkStyleForeground("cpp", SCE_C_STRINGEOL, "SCE_C_STRINGEOL", "error literal string", 0), Theme::Dark::Keyword);
        QCOMPARE(Theme::darkStyleForeground("cpp", SCE_C_NUMBER, "SCE_C_NUMBER", "literal numeric", 0), Theme::Dark::Number);
        QCOMPARE(Theme::darkStyleForeground("lua", SCE_LUA_WORD2, "SCE_LUA_WORD2", "identifier", 0), Theme::Dark::Function);
        // The console reserves style 39 for errors, outside Lua's named styles.
        QCOMPARE(Theme::darkStyleForeground("lua", 39, {}, {}, 0x0000ff), Theme::Dark::Keyword);
    }

    void lexersWithoutMetadata()
    {
        QCOMPARE(Theme::darkStyleForeground("toml", SCE_TOML_COMMENT, {}, {}, 0), Theme::Dark::Muted);
        QCOMPARE(Theme::darkStyleForeground("toml", SCE_TOML_TABLE, {}, {}, 0), Theme::Dark::Function);
        QCOMPARE(Theme::darkStyleForeground("toml", SCE_TOML_KEY, {}, {}, 0), Theme::Dark::Foreground);
        QCOMPARE(Theme::darkStyleForeground("toml", SCE_TOML_KEYWORD, {}, {}, 0), Theme::Dark::Keyword);
        QCOMPARE(Theme::darkStyleForeground("toml", SCE_TOML_TRIPLE_STRING_DQ, {}, {}, 0), Theme::Dark::String);
        for (int style = SCE_MARKDOWN_HEADER1; style <= SCE_MARKDOWN_HEADER6; ++style)
            QCOMPARE(Theme::darkStyleForeground("markdown", style, {}, {}, 0), Theme::Dark::Heading);
        QCOMPARE(Theme::darkStyleForeground("markdown", SCE_MARKDOWN_CODEBK, {}, {}, 0), Theme::Dark::String);
        QCOMPARE(Theme::darkStyleForeground("markdown", SCE_MARKDOWN_LINK, {}, {}, 0), Theme::Dark::Accent);
        QCOMPARE(Theme::darkStyleForeground("null", 0, {}, {}, 0), Theme::Dark::Foreground);
    }

    void stylesheetChangesColorsOnly()
    {
        const QString sheet = Theme::widgetStyleSheet(true);
        QVERIFY(sheet.contains("#0d1117"));
        QVERIFY(sheet.contains("#010409"));
        QVERIFY(sheet.contains("#f78166"));
        QVERIFY(!sheet.contains(QRegularExpression("%[0-9]+")));
        QVERIFY(!sheet.contains(QRegularExpression("(?:^|[;{])\\s*(?:font[^:]*|padding[^:]*|margin[^:]*|width|height|border-width)\\s*:")));
    }
};

QTEST_GUILESS_MAIN(ThemePaletteTest)
#include "ThemePaletteTest.moc"
