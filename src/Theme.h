#pragma once

#include <QByteArray>
#include <QString>

class ScintillaNext;
class QPalette;

namespace Theme {
// RGB values from the approved GitHub previews, shared by Qt and Scintilla.
// Only the API boundary converts these to Scintilla BGR.
namespace Light {
inline constexpr int Background = 0xffffff;
inline constexpr int Surface = 0xf6f8fa;
inline constexpr int Inset = 0xeff2f5;
inline constexpr int Raised = 0xe6eaef;
inline constexpr int Border = 0xd1d9e0;
inline constexpr int Foreground = 0x1f2328;
inline constexpr int Muted = 0x59636e;
inline constexpr int Disabled = 0x818b98;
inline constexpr int Accent = 0x0969da;
inline constexpr int Selection = 0xcce5ff;
inline constexpr int Keyword = 0xcf222e;
inline constexpr int String = 0x0a3069;
inline constexpr int Number = 0x0550ae;
inline constexpr int Function = 0x8250df;
// Syntax-specific roles from GitHub's VS Code Light Default theme. Muted
// above remains the interface color, independent of the comment color.
inline constexpr int Comment = 0x6e7781;
inline constexpr int Entity = 0x953800;
inline constexpr int Tag = 0x116329;
inline constexpr int Heading = 0x0550ae;
inline constexpr int Quote = 0x116329;
inline constexpr int Code = 0x0550ae;
inline constexpr int Error = 0x82071e;
inline constexpr int SearchBackground = 0xfff8c5;
inline constexpr int SearchText = 0x7d4e00;
inline constexpr int TabAccent = 0xfd8c73;
}

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
int lightStyleForeground(const QByteArray &lexer, int style, const QByteArray &name,
                         const QByteArray &tags, int originalBgr);
// Restore the original colors before changing a document's language styles.
void restoreEditorColors(ScintillaNext *editor);
// Apply either GitHub Dark or GitHub Light, always from the original styles.
void setEditorDark(ScintillaNext *editor, bool dark);
}
