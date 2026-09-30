#include "Theme.h"
#include "ScintillaNext.h"

#include <QVariant>

namespace {
constexpr auto savedColors = "nn_baseColors";
constexpr auto appliedTheme = "nn_appliedDarkTheme";
constexpr int elements[] = {
    SC_ELEMENT_CARET, SC_ELEMENT_CARET_ADDITIONAL,
    SC_ELEMENT_CARET_LINE_BACK, SC_ELEMENT_WHITE_SPACE,
    SC_ELEMENT_SELECTION_BACK, SC_ELEMENT_SELECTION_INACTIVE_BACK,
    SC_ELEMENT_SELECTION_ADDITIONAL_BACK, SC_ELEMENT_SELECTION_TEXT,
    SC_ELEMENT_SELECTION_ADDITIONAL_TEXT, SC_ELEMENT_SELECTION_INACTIVE_TEXT,
    SC_ELEMENT_FOLD_LINE
};
}

void Theme::restoreEditorColors(ScintillaNext *editor)
{
    const QVariantList colors = editor->QObject::property(savedColors).toList();
    if (colors.isEmpty())
        return;
    int i = 0;
    for (int style = 0; style <= STYLE_MAX; ++style) {
        editor->styleSetFore(style, colors[i++].toInt());
        editor->styleSetBack(style, colors[i++].toInt());
    }
    for (int element : elements) {
        const bool wasSet = colors[i++].toBool();
        const auto color = colors[i++].toLongLong();
        if (wasSet)
            editor->setElementColour(element, color);
        else
            editor->resetElementColour(element);
    }
    editor->setEdgeColour(colors[i].toInt());
    editor->QObject::setProperty(savedColors, QVariant());
    editor->QObject::setProperty(appliedTheme, QVariant());
}

void Theme::setEditorDark(ScintillaNext *editor, bool dark)
{
    const QVariant previousTheme = editor->QObject::property(appliedTheme);
    if (previousTheme.isValid() && previousTheme.toBool() == dark)
        return;

    // Both palettes are transformations of the original language styles.
    // Restore them first instead of mapping GitHub Light colors into Dark
    // (or vice versa), especially for lexers using the original-hue fallback.
    restoreEditorColors(editor);
    QVariantList colors;
    const QByteArray lexer = editor->lexerLanguage();
    const auto namedStyles = editor->namedStyles();
    const auto styleForeground = dark ? darkStyleForeground : lightStyleForeground;
    const int background = dark ? Dark::Background : Light::Background;
    const int foreground = dark ? Dark::Foreground : Light::Foreground;
    const int surface = dark ? Dark::Surface : Light::Surface;
    const int raised = dark ? Dark::Raised : Light::Raised;
    const int border = dark ? Dark::Border : Light::Border;
    const int muted = dark ? Dark::Muted : Light::Muted;
    const int disabled = dark ? Dark::Disabled : Light::Disabled;
    const int accent = dark ? Dark::Accent : Light::Accent;
    const int keyword = dark ? Dark::Keyword : Light::Keyword;
    const int selection = dark ? Dark::Selection : Light::Selection;

    for (int style = 0; style <= STYLE_MAX; ++style) {
        const int originalForeground = editor->styleFore(style);
        colors << originalForeground << int(editor->styleBack(style));
        const QByteArray name = style < namedStyles ? editor->nameOfStyle(style) : QByteArray();
        const QByteArray tags = style < namedStyles ? editor->tagsOfStyle(style) : QByteArray();
        editor->styleSetFore(style, scintillaColour(styleForeground(lexer, style, name, tags, originalForeground)));
        editor->styleSetBack(style, scintillaColour(background));
    }
    for (int element : elements) {
        colors << editor->elementIsSet(element) << qlonglong(editor->elementColour(element));
    }
    colors << int(editor->edgeColour());
    editor->QObject::setProperty(savedColors, colors);
    editor->QObject::setProperty(appliedTheme, dark);

    editor->styleSetBack(STYLE_LINENUMBER, scintillaColour(background));
    editor->styleSetFore(STYLE_LINENUMBER, scintillaColour(muted));
    editor->styleSetFore(STYLE_INDENTGUIDE, scintillaColour(border));
    editor->styleSetFore(STYLE_BRACELIGHT, scintillaColour(accent));
    editor->styleSetBack(STYLE_BRACELIGHT, scintillaColour(raised));
    editor->styleSetFore(STYLE_BRACEBAD, scintillaColour(keyword));
    editor->styleSetBack(STYLE_BRACEBAD, scintillaColour(raised));
    editor->setElementColour(SC_ELEMENT_CARET, scintillaElementColour(foreground));
    editor->setElementColour(SC_ELEMENT_CARET_ADDITIONAL, scintillaElementColour(foreground));
    editor->setElementColour(SC_ELEMENT_CARET_LINE_BACK, scintillaElementColour(surface));
    editor->setElementColour(SC_ELEMENT_WHITE_SPACE, scintillaElementColour(disabled));
    editor->setElementColour(SC_ELEMENT_SELECTION_BACK, scintillaElementColour(selection));
    editor->setElementColour(SC_ELEMENT_SELECTION_INACTIVE_BACK, scintillaElementColour(raised));
    editor->setElementColour(SC_ELEMENT_SELECTION_ADDITIONAL_BACK, scintillaElementColour(selection));
    editor->setElementColour(SC_ELEMENT_SELECTION_TEXT, scintillaElementColour(foreground));
    editor->setElementColour(SC_ELEMENT_SELECTION_ADDITIONAL_TEXT, scintillaElementColour(foreground));
    editor->setElementColour(SC_ELEMENT_SELECTION_INACTIVE_TEXT, scintillaElementColour(foreground));
    editor->setElementColour(SC_ELEMENT_FOLD_LINE, scintillaElementColour(border));
    editor->setEdgeColour(scintillaColour(border));
    editor->setFoldMarginColour(true, scintillaColour(background));
    editor->setFoldMarginHiColour(true, scintillaColour(background));
    editor->callTipSetBack(scintillaColour(raised));
    editor->callTipSetFore(scintillaColour(foreground));
    editor->callTipSetForeHlt(scintillaColour(accent));
    editor->update();
}
