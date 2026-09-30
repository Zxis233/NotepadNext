#include "Theme.h"
#include "SciLexer.h"

#include <QColor>
#include <QList>
#include <QPalette>

QPalette Theme::applicationPalette(bool dark)
{
    // Neither palette depends on the system's appearance at application start.
    QPalette colors(QColor(dark ? Dark::Raised : Light::Surface));
    colors.setColor(QPalette::Window, QColor(dark ? Dark::Surface : Light::Surface));
    colors.setColor(QPalette::WindowText, QColor(dark ? Dark::Foreground : Light::Foreground));
    colors.setColor(QPalette::Base, QColor(dark ? Dark::Background : Light::Background));
    colors.setColor(QPalette::AlternateBase, QColor(dark ? Dark::Surface : Light::Surface));
    colors.setColor(QPalette::Text, QColor(dark ? Dark::Foreground : Light::Foreground));
    colors.setColor(QPalette::Button, QColor(dark ? Dark::Raised : Light::Surface));
    colors.setColor(QPalette::ButtonText, QColor(dark ? Dark::Foreground : Light::Foreground));
    colors.setColor(QPalette::ToolTipBase, QColor(dark ? Dark::Raised : Light::Background));
    colors.setColor(QPalette::ToolTipText, QColor(dark ? Dark::Foreground : Light::Foreground));
    colors.setColor(QPalette::Highlight, QColor(dark ? Dark::Selection : Light::Selection));
    colors.setColor(QPalette::HighlightedText, QColor(dark ? Dark::Foreground : Light::Foreground));
    colors.setColor(QPalette::Link, QColor(dark ? Dark::Accent : Light::Accent));
    colors.setColor(QPalette::LinkVisited, QColor(dark ? Dark::Function : Light::Function));
    colors.setColor(QPalette::BrightText, QColor(dark ? Dark::Keyword : Light::Keyword));
    colors.setColor(QPalette::PlaceholderText, QColor(dark ? Dark::Muted : Light::Muted));
    colors.setColor(QPalette::Light, QColor(dark ? Dark::Border : Light::Background));
    colors.setColor(QPalette::Midlight, QColor(dark ? Dark::Raised : Light::Raised));
    colors.setColor(QPalette::Mid, QColor(dark ? Dark::Border : Light::Border));
    colors.setColor(QPalette::Dark, QColor(dark ? Dark::Inset : Light::Muted));
    colors.setColor(QPalette::Shadow, QColor(dark ? Dark::Inset : Light::Muted));
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        colors.setColor(QPalette::Disabled, role, QColor(dark ? Dark::Disabled : Light::Disabled));
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    colors.setColor(QPalette::Accent, QColor(dark ? Dark::Accent : Light::Accent));
#endif
    return colors;
}

QString Theme::widgetStyleSheet(bool dark)
{
    // Color overrides only: keep all existing Fusion/QSS layout metrics.
    // MainWindow appends custom.css after these rules, preserving user overrides.
    return QStringLiteral(R"(
QStatusBar { background-color: %2; color: %5; border-top-color: %6; }
QStatusBar QLabel { color: %5; }
ads--CDockAreaTitleBar, ads--CDockAreaTabBar { background-color: %3; border-bottom-color: %6; }
ads--CDockWidgetTab { background-color: %3; border-left-color: %6; border-right-color: %6; border-top-color: %6; }
ads--CDockWidgetTab ads--CElidingLabel { color: %4; }
ads--CDockWidgetTab[activeTab="false"] QLabel { color: %5; }
ads--CDockWidgetTab[activeTab="true"], ads--CDockWidgetTab[focused="true"] { background-color: %1; border-top-color: %8; }
ads--CDockWidgetTab:hover[activeTab="false"] { background-color: %2; }
#tabCloseButton { background-color: transparent; color: %5; }
#tabCloseButton:hover { background-color: %7; border-color: %6; color: %4; }
#tabCloseButton:pressed { background-color: %7; color: %4; }
#QuickFindWidget { background-color: %2; border-left-color: %6; border-right-color: %6; border-bottom-color: %9; }
)")
        .arg(QColor(dark ? Dark::Background : Light::Background).name(),
             QColor(dark ? Dark::Surface : Light::Surface).name(),
             QColor(dark ? Dark::Inset : Light::Inset).name(),
             QColor(dark ? Dark::Foreground : Light::Foreground).name(),
             QColor(dark ? Dark::Muted : Light::Muted).name(),
             QColor(dark ? Dark::Border : Light::Border).name(),
             QColor(dark ? Dark::Raised : Light::Raised).name(),
             QColor(dark ? Dark::TabAccent : Light::TabAccent).name(),
             QColor(dark ? Dark::Accent : Light::Accent).name());
}

int Theme::darkStyleForeground(const QByteArray &lexer, int style, const QByteArray &name,
                              const QByteArray &tags, int lightBgr)
{
    // These legacy lexers do not expose nameOfStyle/tagsOfStyle metadata.
    if (lexer == "toml") {
        switch (style) {
        case SCE_TOML_COMMENT: return Dark::Muted;
        case SCE_TOML_KEYWORD:
        case SCE_TOML_ERROR:
        case SCE_TOML_STRINGEOL: return Dark::Keyword;
        case SCE_TOML_NUMBER:
        case SCE_TOML_DATETIME:
        case SCE_TOML_ESCAPECHAR: return Dark::Number;
        case SCE_TOML_TABLE: return Dark::Function;
        case SCE_TOML_STRING_SQ:
        case SCE_TOML_STRING_DQ:
        case SCE_TOML_TRIPLE_STRING_SQ:
        case SCE_TOML_TRIPLE_STRING_DQ: return Dark::String;
        default: return Dark::Foreground;
        }
    }
    if (lexer == "markdown") {
        if (style >= SCE_MARKDOWN_HEADER1 && style <= SCE_MARKDOWN_HEADER6)
            return Dark::Heading;
        switch (style) {
        case SCE_MARKDOWN_BLOCKQUOTE:
        case SCE_MARKDOWN_HRULE: return Dark::Muted;
        case SCE_MARKDOWN_LINK: return Dark::Accent;
        case SCE_MARKDOWN_CODE:
        case SCE_MARKDOWN_CODE2:
        case SCE_MARKDOWN_CODEBK: return Dark::String;
        default: return Dark::Foreground;
        }
    }

    const auto words = tags.toLower().split(' ');
    const QByteArray lowerName = name.toLower();
    if (words.contains("error") || lowerName.contains("error") || lowerName.endsWith("stringeol"))
        return Dark::Keyword;
    if (words.contains("comment") || lowerName.contains("comment"))
        return Dark::Muted;
    if (words.contains("string") || words.contains("regex") || lowerName.contains("string") ||
        lowerName.contains("character") || lowerName.contains("regex"))
        return Dark::String;
    if (words.contains("numeric") || words.contains("number") || lowerName.contains("number"))
        return Dark::Number;
    if (lexer == "lua" && style >= SCE_LUA_WORD2 && style <= SCE_LUA_WORD8)
        return Dark::Function;
    if (words.contains("keyword") || words.contains("preprocessor") || lowerName.contains("word"))
        return Dark::Keyword;
    if (words.contains("function") || words.contains("method") || words.contains("label") ||
        lowerName.contains("function") || lowerName.contains("globalclass"))
        return Dark::Function;
    if (!tags.isEmpty())
        return Dark::Foreground; // Do not color every identifier as a function.

    // Preserve a useful distinction for lexers without metadata, but map their
    // old hues onto the same GitHub palette instead of merely brightening them.
    const QColor original(lightBgr & 255, (lightBgr >> 8) & 255, (lightBgr >> 16) & 255);
    if (original.hslSaturationF() < 0.15)
        return Dark::Foreground;
    const int hue = original.hslHue();
    if (hue < 25 || hue >= 345)
        return original.lightnessF() < 0.4 ? Dark::String : Dark::Keyword;
    if (hue < 75) return Dark::Number;
    if (hue < 165) return Dark::Muted;
    if (hue < 205) return Dark::Number;
    if (hue < 265) return Dark::Keyword;
    return Dark::Function;
}

int Theme::lightStyleForeground(const QByteArray &lexer, int style, const QByteArray &name,
                               const QByteArray &tags, int originalBgr)
{
    // Match the scopes in primer/github-vscode-theme's Light Default theme.
    // Legacy lexers lack metadata, but still expose meaningful style IDs.
    if (lexer == "json") {
        switch (style) {
        case SCE_JSON_PROPERTYNAME: return Light::Tag;
        case SCE_JSON_NUMBER:
        case SCE_JSON_KEYWORD:
        case SCE_JSON_LDKEYWORD: return Light::Number;
        case SCE_JSON_STRING:
        case SCE_JSON_URI:
        case SCE_JSON_COMPACTIRI: return Light::String;
        case SCE_JSON_ESCAPESEQUENCE: return Light::Keyword;
        case SCE_JSON_LINECOMMENT:
        case SCE_JSON_BLOCKCOMMENT: return Light::Comment;
        case SCE_JSON_STRINGEOL:
        case SCE_JSON_ERROR: return Light::Error;
        default: return Light::Foreground;
        }
    }
    if (lexer == "toml") {
        switch (style) {
        case SCE_TOML_TABLE: return Light::Entity;
        case SCE_TOML_COMMENT: return Light::Comment;
        case SCE_TOML_KEYWORD: // true, false, inf, nan are constants, not control keywords.
        case SCE_TOML_NUMBER:
        case SCE_TOML_DATETIME: return Light::Number;
        case SCE_TOML_STRING_SQ:
        case SCE_TOML_STRING_DQ:
        case SCE_TOML_TRIPLE_STRING_SQ:
        case SCE_TOML_TRIPLE_STRING_DQ: return Light::String;
        case SCE_TOML_ESCAPECHAR: return Light::Keyword;
        case SCE_TOML_STRINGEOL:
        case SCE_TOML_ERROR: return Light::Error;
        default: return Light::Foreground;
        }
    }
    if (lexer == "markdown") {
        if (style >= SCE_MARKDOWN_HEADER1 && style <= SCE_MARKDOWN_HEADER6)
            return Light::Heading;
        switch (style) {
        case SCE_MARKDOWN_BLOCKQUOTE: return Light::Quote;
        case SCE_MARKDOWN_CODE:
        case SCE_MARKDOWN_CODE2:
        case SCE_MARKDOWN_CODEBK: return Light::Code;
        case SCE_MARKDOWN_ULIST_ITEM:
        case SCE_MARKDOWN_OLIST_ITEM: return Light::Entity;
        case SCE_MARKDOWN_LINK: return Light::Number;
        case SCE_MARKDOWN_HRULE: return Light::Number;
        default: return Light::Foreground;
        }
    }
    if (lexer == "hypertext" || lexer == "xml") {
        switch (style) {
        // Unknown tags/attributes may simply be custom web components. They
        // are still tags/attributes, not necessarily illegal source text.
        case SCE_H_TAG:
        case SCE_H_TAGUNKNOWN: return Light::Tag;
        case SCE_H_ATTRIBUTE:
        case SCE_H_ATTRIBUTEUNKNOWN:
        case SCE_H_NUMBER: return Light::Number;
        case SCE_H_DOUBLESTRING:
        case SCE_H_SINGLESTRING:
        case SCE_H_VALUE:
        case SCE_H_CDATA: return Light::String;
        case SCE_H_COMMENT:
        case SCE_H_XCCOMMENT: return Light::Comment;
        case SCE_H_ENTITY: return Light::Keyword;
        case SCE_H_TAGEND:
        case SCE_H_XMLSTART:
        case SCE_H_XMLEND:
        case SCE_H_OTHER: return Light::Foreground;
        default: break; // Embedded scripts use their own metadata below.
        }
    }
    if (lexer == "python") {
        switch (style) {
        case SCE_P_CLASSNAME: return Light::Entity;
        case SCE_P_DEFNAME:
        case SCE_P_DECORATOR: return Light::Function;
        case SCE_P_WORD2: return Light::Number;
        default: break;
        }
    }
    if (lexer == "lua" && style >= SCE_LUA_WORD2 && style <= SCE_LUA_WORD8)
        return Light::Number; // support scopes (built-in/library names)
    if (lexer == "lua" && style == 39 && originalBgr == 0x0000ff)
        return Light::Error; // Lua console's custom message.error style

    const auto words = tags.toLower().split(' ');
    const QByteArray lowerName = name.toLower();
    if (words.contains("error") || lowerName.contains("error") || lowerName.endsWith("stringeol"))
        return Light::Error;
    if (words.contains("comment") || lowerName.contains("comment"))
        return Light::Comment;
    if (words.contains("tag"))
        return Light::Tag;
    if (words.contains("attribute") || words.contains("constant") || words.contains("numeric") ||
        words.contains("number") || lowerName.contains("number"))
        return Light::Number;
    if (lowerName.contains("escape"))
        return Light::Keyword;
    if (words.contains("string") || words.contains("regex") || lowerName.contains("string") ||
        lowerName.contains("character") || lowerName.contains("regex"))
        return Light::String;
    if (words.contains("keyword") || words.contains("preprocessor") || lowerName.contains("word"))
        return Light::Keyword;
    if (words.contains("function") || words.contains("method") ||
        lowerName.contains("function") || lowerName.contains("method"))
        return Light::Function;
    if (words.contains("class") || words.contains("type") || words.contains("label") || lowerName.contains("class"))
        return Light::Entity;
    if (!tags.isEmpty())
        return Light::Foreground;

    // Without a distinct style, ordinary identifiers must remain neutral.
    // In particular, C++'s shared identifier style cannot distinguish arbitrary
    // function names, user-defined types and variables as TextMate/LSP can.
    switch (darkStyleForeground(lexer, style, name, tags, originalBgr)) {
    case Dark::Muted: return Light::Comment;
    case Dark::Keyword: return Light::Keyword;
    case Dark::String: return Light::String;
    case Dark::Number: return Light::Number;
    case Dark::Function: return Light::Function;
    case Dark::Heading: return Light::Heading;
    case Dark::Accent: return Light::Accent;
    default: return Light::Foreground;
    }
}
