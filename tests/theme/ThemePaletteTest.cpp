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
        QCOMPARE(Theme::scintillaColour(0x1f2328), 0x28231f);
        QCOMPARE(Theme::scintillaElementColour(0xcce5ff), 0xffffe5ccu);
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

    void githubLightInterfacePalette()
    {
        const QPalette palette = Theme::applicationPalette(false);
        for (auto group : {QPalette::Active, QPalette::Inactive}) {
            QCOMPARE(palette.color(group, QPalette::Window), QColor("#f6f8fa"));
            QCOMPARE(palette.color(group, QPalette::Base), QColor("#ffffff"));
            QCOMPARE(palette.color(group, QPalette::Text), QColor("#1f2328"));
            QCOMPARE(palette.color(group, QPalette::Button), QColor("#f6f8fa"));
            QCOMPARE(palette.color(group, QPalette::Highlight), QColor("#cce5ff"));
            QCOMPARE(palette.color(group, QPalette::HighlightedText), QColor("#1f2328"));
            QCOMPARE(palette.color(group, QPalette::Link), QColor("#0969da"));
        }
        QCOMPARE(palette.color(QPalette::Disabled, QPalette::Text), QColor("#818b98"));
        const QString sheet = Theme::widgetStyleSheet(false);
        QVERIFY(sheet.contains("#eff2f5"));
        QVERIFY(sheet.contains("#fd8c73"));
        QVERIFY(sheet.contains("#d1d9e0"));
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

    void lightSyntaxUsesGithubLightDefault()
    {
        QCOMPARE(Theme::lightStyleForeground("cpp", SCE_C_COMMENT, "SCE_C_COMMENT", "comment", 0x008000), 0x6e7781);
        QCOMPARE(Theme::lightStyleForeground("cpp", SCE_C_WORD2, "SCE_C_WORD2", "identifier", 0), Theme::Light::Keyword);
        QCOMPARE(Theme::lightStyleForeground("cpp", SCE_C_STRING, "SCE_C_STRING", "literal string", 0x808080), Theme::Light::String);
        QCOMPARE(Theme::lightStyleForeground("cpp", SCE_C_NUMBER, "SCE_C_NUMBER", "literal numeric", 0), Theme::Light::Number);
        QCOMPARE(Theme::lightStyleForeground("toml", SCE_TOML_TABLE, {}, {}, 0), 0x953800);
        QCOMPARE(Theme::lightStyleForeground("toml", SCE_TOML_KEY, {}, {}, 0), Theme::Light::Foreground);
        QCOMPARE(Theme::lightStyleForeground("toml", SCE_TOML_KEYWORD, {}, {}, 0), 0x0550ae);
        QCOMPARE(Theme::lightStyleForeground("toml", SCE_TOML_STRING_DQ, {}, {}, 0), 0x0a3069);
        QCOMPARE(Theme::lightStyleForeground("markdown", SCE_MARKDOWN_HEADER1, {}, {}, 0), 0x0550ae);
        QCOMPARE(Theme::lightStyleForeground("markdown", SCE_MARKDOWN_BLOCKQUOTE, {}, {}, 0), 0x116329);
        QCOMPARE(Theme::lightStyleForeground("markdown", SCE_MARKDOWN_CODE, {}, {}, 0), 0x0550ae);
        QCOMPARE(Theme::lightStyleForeground("markdown", SCE_MARKDOWN_HRULE, {}, {}, 0), 0x0550ae);
        QCOMPARE(Theme::lightStyleForeground("markdown", SCE_MARKDOWN_LINK, {}, {}, 0), Theme::Light::Number);
        QCOMPARE(Theme::lightStyleForeground("lua", 39, {}, {}, 0x0000ff), Theme::Light::Error);
        // The original maroon (BGR) maps to a string in both themes. Passing
        // an already transformed blue into the fallback would misclassify it.
        QCOMPARE(Theme::lightStyleForeground("legacy", 0, {}, {}, 0x000080), Theme::Light::String);
        QCOMPARE(Theme::darkStyleForeground("legacy", 0, {}, {}, 0x000080), Theme::Dark::String);
    }

    void lightJsonDistinguishesKeysFromStrings()
    {
        QCOMPARE(Theme::lightStyleForeground("json", SCE_JSON_PROPERTYNAME, {}, {}, 0), 0x116329);
        QCOMPARE(Theme::lightStyleForeground("json", SCE_JSON_STRING, {}, {}, 0), 0x0a3069);
        QCOMPARE(Theme::lightStyleForeground("json", SCE_JSON_NUMBER, {}, {}, 0), 0x0550ae);
        QCOMPARE(Theme::lightStyleForeground("json", SCE_JSON_KEYWORD, {}, {}, 0), 0x0550ae);
        QCOMPARE(Theme::lightStyleForeground("json", SCE_JSON_BLOCKCOMMENT, {}, {}, 0), 0x6e7781);
        QCOMPARE(Theme::lightStyleForeground("json", SCE_JSON_ERROR, {}, {}, 0), 0x82071e);
    }

    void lightHtmlDistinguishesTagsAndAttributes()
    {
        for (const QByteArray lexer : {QByteArray("hypertext"), QByteArray("xml")}) {
            QCOMPARE(Theme::lightStyleForeground(lexer, SCE_H_TAG, "Tags", "tag", 0), 0x116329);
            QCOMPARE(Theme::lightStyleForeground(lexer, SCE_H_ATTRIBUTE, "Attributes", "attribute", 0), 0x0550ae);
            QCOMPARE(Theme::lightStyleForeground(lexer, SCE_H_DOUBLESTRING, {}, {}, 0), 0x0a3069);
            QCOMPARE(Theme::lightStyleForeground(lexer, SCE_H_TAGUNKNOWN, "Unknown Tags", "error tag", 0), 0x116329);
            QCOMPARE(Theme::lightStyleForeground(lexer, SCE_H_TAGEND, {}, {}, 0), Theme::Light::Foreground);
        }
    }

    void lightUsesOnlyAvailableIdentifierCategories()
    {
        QCOMPARE(Theme::lightStyleForeground("python", SCE_P_CLASSNAME, "Class name definition", "identifier", 0), 0x953800);
        QCOMPARE(Theme::lightStyleForeground("python", SCE_P_DEFNAME, "Function or method name definition", "identifier", 0), 0x8250df);
        QCOMPARE(Theme::lightStyleForeground("python", SCE_P_IDENTIFIER, "Identifiers", "identifier", 0), Theme::Light::Foreground);
        QCOMPARE(Theme::lightStyleForeground("cpp", SCE_C_IDENTIFIER, "SCE_C_IDENTIFIER", "identifier", 0), Theme::Light::Foreground);
        QCOMPARE(Theme::lightStyleForeground("cpp", SCE_C_GLOBALCLASS, "SCE_C_GLOBALCLASS", "identifier", 0), 0x953800);
        QCOMPARE(Theme::lightStyleForeground("lua", SCE_LUA_WORD2, "SCE_LUA_WORD2", "identifier", 0), 0x0550ae);
    }

    void stylesheetChangesColorsOnly()
    {
        const QString darkSheet = Theme::widgetStyleSheet(true);
        QVERIFY(darkSheet.contains("#0d1117"));
        QVERIFY(darkSheet.contains("#010409"));
        QVERIFY(darkSheet.contains("#f78166"));
        for (bool dark : {false, true}) {
            const QString sheet = Theme::widgetStyleSheet(dark);
            QVERIFY(!sheet.contains(QRegularExpression("%[0-9]+")));
            QVERIFY(!sheet.contains(QRegularExpression("(?:^|[;{])\\s*(?:font[^:]*|padding[^:]*|margin[^:]*|width|height|border-width)\\s*:")));
        }
    }
};

QTEST_GUILESS_MAIN(ThemePaletteTest)
#include "ThemePaletteTest.moc"
