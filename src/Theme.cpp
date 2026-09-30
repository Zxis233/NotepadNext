#include "Theme.h"
#include "ScintillaNext.h"

#include <QColor>
#include <QVariant>

namespace {
constexpr auto savedColors = "nn_lightColors";
constexpr int elements[] = {
    SC_ELEMENT_CARET, SC_ELEMENT_CARET_ADDITIONAL,
    SC_ELEMENT_CARET_LINE_BACK, SC_ELEMENT_WHITE_SPACE,
    SC_ELEMENT_SELECTION_BACK, SC_ELEMENT_SELECTION_INACTIVE_BACK,
    SC_ELEMENT_SELECTION_ADDITIONAL_BACK, SC_ELEMENT_SELECTION_TEXT,
    SC_ELEMENT_SELECTION_ADDITIONAL_TEXT, SC_ELEMENT_FOLD_LINE
};

int readableForeground(int bgr)
{
    QColor color(bgr & 255, (bgr >> 8) & 255, (bgr >> 16) & 255);
    // Keep the language's hue while lifting dark syntax colors off the background.
    if (color.lightnessF() < 0.65)
        color.setHslF(color.hslHueF(), color.hslSaturationF(), 0.72);
    return color.red() | (color.green() << 8) | (color.blue() << 16);
}
}

void Theme::setEditorDark(ScintillaNext *editor, bool dark)
{
    QVariantList colors = editor->QObject::property(savedColors).toList();
    if (dark == !colors.isEmpty())
        return;

    if (dark) {
        for (int style = 0; style <= STYLE_MAX; ++style) {
            colors << int(editor->styleFore(style)) << int(editor->styleBack(style));
            editor->styleSetFore(style, readableForeground(editor->styleFore(style)));
            editor->styleSetBack(style, 0x202020);
        }
        for (int element : elements) {
            colors << editor->elementIsSet(element) << qlonglong(editor->elementColour(element));
        }
        colors << int(editor->edgeColour());
        editor->QObject::setProperty(savedColors, colors);
        editor->styleSetBack(STYLE_LINENUMBER, 0x292929);
        editor->styleSetFore(STYLE_LINENUMBER, 0xA0A0A0);
        editor->setElementColour(SC_ELEMENT_CARET, 0xFFF0F0F0);
        editor->setElementColour(SC_ELEMENT_CARET_ADDITIONAL, 0xFFF0F0F0);
        editor->setElementColour(SC_ELEMENT_CARET_LINE_BACK, 0xFF303030);
        editor->setElementColour(SC_ELEMENT_WHITE_SPACE, 0xFF707070);
        editor->setElementColour(SC_ELEMENT_SELECTION_BACK, 0xFF805030);
        editor->setElementColour(SC_ELEMENT_SELECTION_INACTIVE_BACK, 0xFF505050);
        editor->setElementColour(SC_ELEMENT_SELECTION_ADDITIONAL_BACK, 0xFF705030);
        editor->setElementColour(SC_ELEMENT_SELECTION_TEXT, 0xFFF0F0F0);
        editor->setElementColour(SC_ELEMENT_SELECTION_ADDITIONAL_TEXT, 0xFFF0F0F0);
        editor->setElementColour(SC_ELEMENT_FOLD_LINE, 0xFF707070);
        editor->setEdgeColour(0x606060);
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
    editor->setFoldMarginColour(true, dark ? 0x292929 : 0xFFFFFF);
    editor->setFoldMarginHiColour(true, dark ? 0x292929 : 0xE9E9E9);
    editor->callTipSetBack(dark ? 0x303030 : 0xFFFFFF);
    editor->callTipSetFore(dark ? 0xE0E0E0 : 0x000000);
    editor->callTipSetForeHlt(dark ? 0xFFC080 : 0x800000);
    editor->update();
}
