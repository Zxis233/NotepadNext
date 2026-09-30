#include "Theme.h"
#include "SciLexer.h"

#include <QColor>
#include <QList>
#include <QPalette>

QPalette Theme::applicationPalette(bool dark)
{
    // Neither palette depends on the system's appearance at application start.
    QPalette colors(QColor(dark ? Dark::Raised : 0xf0f0f0));
    colors.setColor(QPalette::Window, QColor(dark ? Dark::Surface : 0xf0f0f0));
    colors.setColor(QPalette::WindowText, QColor(dark ? Dark::Foreground : 0x000000));
    colors.setColor(QPalette::Base, QColor(dark ? Dark::Background : 0xffffff));
    colors.setColor(QPalette::AlternateBase, QColor(dark ? Dark::Surface : 0xf7f7f7));
    colors.setColor(QPalette::Text, QColor(dark ? Dark::Foreground : 0x000000));
    colors.setColor(QPalette::Button, QColor(dark ? Dark::Raised : 0xf0f0f0));
    colors.setColor(QPalette::ButtonText, QColor(dark ? Dark::Foreground : 0x000000));
    colors.setColor(QPalette::ToolTipBase, QColor(dark ? Dark::Raised : 0xffffdc));
    colors.setColor(QPalette::ToolTipText, QColor(dark ? Dark::Foreground : 0x000000));
    colors.setColor(QPalette::Highlight, QColor(dark ? Dark::Selection : 0x308cc6));
    colors.setColor(QPalette::HighlightedText, QColor(dark ? Dark::Foreground : 0xffffff));
    colors.setColor(QPalette::Link, QColor(dark ? Dark::Accent : 0x0000ff));
    colors.setColor(QPalette::LinkVisited, QColor(dark ? Dark::Function : 0x800080));
    colors.setColor(QPalette::BrightText, QColor(dark ? Dark::Keyword : 0xff0000));
    colors.setColor(QPalette::PlaceholderText, QColor(dark ? Dark::Muted : 0x767676));
    colors.setColor(QPalette::Light, QColor(dark ? Dark::Border : 0xffffff));
    colors.setColor(QPalette::Midlight, QColor(dark ? Dark::Raised : 0xe3e3e3));
    colors.setColor(QPalette::Mid, QColor(dark ? Dark::Border : 0xa0a0a0));
    colors.setColor(QPalette::Dark, QColor(dark ? Dark::Inset : 0x808080));
    colors.setColor(QPalette::Shadow, QColor(dark ? Dark::Inset : 0x696969));
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        colors.setColor(QPalette::Disabled, role, QColor(dark ? Dark::Disabled : 0x808080));
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    colors.setColor(QPalette::Accent, QColor(dark ? Dark::Accent : 0x308cc6));
#endif
    return colors;
}

QString Theme::widgetStyleSheet(bool dark)
{
    if (!dark)
        return {};
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
        .arg(QColor(Dark::Background).name(), QColor(Dark::Surface).name(), QColor(Dark::Inset).name(),
             QColor(Dark::Foreground).name(), QColor(Dark::Muted).name(), QColor(Dark::Border).name(),
             QColor(Dark::Raised).name(), QColor(Dark::TabAccent).name(), QColor(Dark::Accent).name());
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
