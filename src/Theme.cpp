#include "Theme.h"
#include "ScintillaNext.h"

#include <QVariant>

namespace {
constexpr auto savedColors = "nn_lightColors";
constexpr int elements[] = {
    SC_ELEMENT_CARET, SC_ELEMENT_CARET_ADDITIONAL,
    SC_ELEMENT_CARET_LINE_BACK, SC_ELEMENT_WHITE_SPACE,
    SC_ELEMENT_SELECTION_BACK, SC_ELEMENT_SELECTION_INACTIVE_BACK,
    SC_ELEMENT_SELECTION_ADDITIONAL_BACK, SC_ELEMENT_SELECTION_TEXT,
    SC_ELEMENT_SELECTION_ADDITIONAL_TEXT, SC_ELEMENT_SELECTION_INACTIVE_TEXT,
    SC_ELEMENT_FOLD_LINE
};
}

void Theme::setEditorDark(ScintillaNext *editor, bool dark)
{
    QVariantList colors = editor->QObject::property(savedColors).toList();
    if (dark == !colors.isEmpty())
        return;

    if (dark) {
        const QByteArray lexer = editor->lexerLanguage();
        const auto namedStyles = editor->namedStyles();
        for (int style = 0; style <= STYLE_MAX; ++style) {
            const int foreground = editor->styleFore(style);
            colors << foreground << int(editor->styleBack(style));
            const QByteArray name = style < namedStyles ? editor->nameOfStyle(style) : QByteArray();
            const QByteArray tags = style < namedStyles ? editor->tagsOfStyle(style) : QByteArray();
            editor->styleSetFore(style, scintillaColour(darkStyleForeground(lexer, style, name, tags, foreground)));
            editor->styleSetBack(style, scintillaColour(Dark::Background));
        }
        for (int element : elements) {
            colors << editor->elementIsSet(element) << qlonglong(editor->elementColour(element));
        }
        colors << int(editor->edgeColour());
        editor->QObject::setProperty(savedColors, colors);
        editor->styleSetBack(STYLE_LINENUMBER, scintillaColour(Dark::Background));
        editor->styleSetFore(STYLE_LINENUMBER, scintillaColour(Dark::Muted));
        editor->styleSetFore(STYLE_INDENTGUIDE, scintillaColour(Dark::Border));
        editor->styleSetFore(STYLE_BRACELIGHT, scintillaColour(Dark::Accent));
        editor->styleSetBack(STYLE_BRACELIGHT, scintillaColour(Dark::Raised));
        editor->styleSetFore(STYLE_BRACEBAD, scintillaColour(Dark::Keyword));
        editor->styleSetBack(STYLE_BRACEBAD, scintillaColour(Dark::Raised));
        editor->setElementColour(SC_ELEMENT_CARET, scintillaElementColour(Dark::Foreground));
        editor->setElementColour(SC_ELEMENT_CARET_ADDITIONAL, scintillaElementColour(Dark::Foreground));
        editor->setElementColour(SC_ELEMENT_CARET_LINE_BACK, scintillaElementColour(Dark::Surface));
        editor->setElementColour(SC_ELEMENT_WHITE_SPACE, scintillaElementColour(Dark::Disabled));
        editor->setElementColour(SC_ELEMENT_SELECTION_BACK, scintillaElementColour(Dark::Selection));
        editor->setElementColour(SC_ELEMENT_SELECTION_INACTIVE_BACK, scintillaElementColour(Dark::Raised));
        editor->setElementColour(SC_ELEMENT_SELECTION_ADDITIONAL_BACK, scintillaElementColour(Dark::Selection));
        editor->setElementColour(SC_ELEMENT_SELECTION_TEXT, scintillaElementColour(Dark::Foreground));
        editor->setElementColour(SC_ELEMENT_SELECTION_ADDITIONAL_TEXT, scintillaElementColour(Dark::Foreground));
        editor->setElementColour(SC_ELEMENT_SELECTION_INACTIVE_TEXT, scintillaElementColour(Dark::Foreground));
        editor->setElementColour(SC_ELEMENT_FOLD_LINE, scintillaElementColour(Dark::Border));
        editor->setEdgeColour(scintillaColour(Dark::Border));
    }
    else {
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
    }
    editor->setFoldMarginColour(true, dark ? scintillaColour(Dark::Background) : 0xFFFFFF);
    editor->setFoldMarginHiColour(true, dark ? scintillaColour(Dark::Background) : 0xE9E9E9);
    editor->callTipSetBack(dark ? scintillaColour(Dark::Raised) : 0xFFFFFF);
    editor->callTipSetFore(dark ? scintillaColour(Dark::Foreground) : 0x000000);
    editor->callTipSetForeHlt(dark ? scintillaColour(Dark::Accent) : 0x800000);
    editor->update();
}
