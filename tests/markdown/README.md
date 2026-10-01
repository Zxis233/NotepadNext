# Markdown lexer regression checks

The app enables `lexer.markdown.gfm=1`. The upstream lexer remains available
when this property is unset. `lexer.markdown.header.eolfill=1` paints complete
ATX headings. Existing Markdown style IDs are preserved; `SCE_MARKDOWN_MATH`
(22) and `SCE_MARKDOWN_MATHBK` (23) distinguish inline and block math.
Both GitHub palettes reuse their existing heading, string and function colors
for headings, code and math respectively. All six heading levels share one
color, while document font settings continue to apply.

Implemented highlighting:

Syntax rules are based on the [GFM specification](https://github.github.com/gfm/).

- ATX headings with valid spacing and one to six markers; single-line setext headings.
- Top-level backtick/tilde fences, optional info strings and up to three leading
  spaces. A closing fence must use the same marker, have at least the opening
  length, and contain only trailing whitespace. Unclosed fences extend to EOF.
- Explicit quote/list prefixes, ordered `.` and `)` markers, and `[ ]`, `[x]`,
  `[X]` task markers. Task boxes reuse their list marker style.
- Top-level pipe tables with matching header/delimiter cell counts, optional
  edge pipes, alignment colons and escaped pipes. Headers use bold, separators
  use the horizontal-rule style, and cells retain inline highlighting.
- Same-line code spans with exactly matching backtick runs, balanced inline
  links/images, explicit reference links, emphasis and strikethrough.
- GitHub math extensions: same-line `$...$` and ``$`...`$`` expressions,
  `$$...$$` expressions, top-level multiline `$$` blocks and `math` fences.
  Escaped dollars and unmatched inline delimiters remain ordinary text;
  single-dollar expressions reject adjacent internal whitespace and a closing
  dollar followed by a digit to avoid highlighting common currency text.
  Code and math retain their own styles inside emphasis and link labels.
  Headings retain their uniform heading style, including inline content.

Math syntax follows [GitHub's mathematical expressions documentation](https://docs.github.com/en/get-started/writing-on-github/working-with-advanced-formatting/writing-mathematical-expressions).
Unclosed math blocks extend to EOF, like unclosed code fences.

With `fold=1`, top-level ATX and single-line setext headings fold their sections
through the next heading of the same or a higher level. Nested headings retain
their heading levels for the existing fold-by-level commands. Empty sections
have no fold button. Setext underlines belong to the title rather than starting
another section. Code/math blocks, quotes and lists do not create fold headers.
Folding follows the actual recolouring range when block edits change which
lines are headings, and propagates section levels until the old context converges.

This is source highlighting, not a full GFM renderer. Embedded code languages,
HTML blocks, indented code blocks, multiline inline constructs, multiline
setext headings, full nested emphasis rules, automatic URL recognition and
fences/tables/math blocks inside continued list or quote containers are not implemented.

The test executable uses the actual bundled lexer and checks token styles and
fold levels, LF/CRLF, missing final newline, byte-range restarts, same-length
edits (with stale styles/states retained), and alternating documents. Its edit
model also shifts styles and per-line metadata for line insertions/deletions.
Actual margin clicks, hidden-line visibility and edit notifications still need
application QA.

To build and run later, from the repository root with a C++20 toolchain:

```powershell
cmake -S tests/markdown -B build/markdown-tests
cmake --build build/markdown-tests --config Release
ctest --test-dir build/markdown-tests -C Release --output-on-failure
```

Qt is not required for this standalone test. With MinGW, put the selected
compiler's `bin` directory first in `PATH` when running CTest so it loads the
matching runtime DLLs. Palette regressions are in `tests/theme` and require Qt.
`GFM-preview.md` is a manual smoke-test document for the editor in both themes.
