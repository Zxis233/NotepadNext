#pragma once

class ScintillaNext;

namespace Theme {
// Restore the original colors before changing a document's language styles.
void setEditorDark(ScintillaNext *editor, bool dark);
}
