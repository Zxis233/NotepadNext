#pragma once

class ScintillaNext;
class QPalette;

namespace Theme {
QPalette applicationPalette(bool dark);
// Restore the original colors before changing a document's language styles.
void setEditorDark(ScintillaNext *editor, bool dark);
}
