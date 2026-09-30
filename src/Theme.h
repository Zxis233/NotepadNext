#pragma once

#include <QByteArray>
#include <QString>

class ScintillaNext;
class QPalette;

namespace Theme {
// RGB values shared by the Qt interface and Scintilla. Matches the approved
// GitHub Dark preview; only the API boundary converts these to Scintilla BGR.
namespace Dark {
inline constexpr int Background = 0x0d1117;
inline constexpr int Surface = 0x151b23;
inline constexpr int Inset = 0x010409;
inline constexpr int Raised = 0x212830;
inline constexpr int Border = 0x3d444d;
inline constexpr int Foreground = 0xf0f6fc;
inline constexpr int Muted = 0x9198a1;
inline constexpr int Disabled = 0x656c76;
inline constexpr int Accent = 0x58a6ff;
inline constexpr int Selection = 0x264f78;
inline constexpr int Keyword = 0xff7b72;
inline constexpr int String = 0xa5d6ff;
inline constexpr int Number = 0x79c0ff;
inline constexpr int Function = 0xd2a8ff;
inline constexpr int Heading = 0x7ee787;
inline constexpr int SearchBackground = 0x5d420a;
inline constexpr int SearchText = 0xf2cc60;
inline constexpr int TabAccent = 0xf78166;
}

constexpr int scintillaColour(int rgb)
{
    return ((rgb & 0xff) << 16) | (rgb & 0xff00) | ((rgb >> 16) & 0xff);
}

constexpr unsigned int scintillaElementColour(int rgb)
{
    return 0xff000000u | static_cast<unsigned int>(scintillaColour(rgb));
}

QPalette applicationPalette(bool dark);
QString widgetStyleSheet(bool dark);
int darkStyleForeground(const QByteArray &lexer, int style, const QByteArray &name,
                        const QByteArray &tags, int lightBgr);
// Restore the original colors before changing a document's language styles.
void setEditorDark(ScintillaNext *editor, bool dark);
}
