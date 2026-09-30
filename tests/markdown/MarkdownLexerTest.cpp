#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ILexer.h"
#include "SciLexer.h"
#include "Lexilla.h"
#include "TestDocument.h"

namespace {

// The upstream test document adds an EOF sentinel line even without a final
// newline. Its LineEnd then drops the last byte; use the real line contents.
class Document : public TestDocument {
public:
    Sci_Position SCI_METHOD LineEnd(Sci_Position line) const override {
        Sci_Position pos = LineStart(line);
        char ch = 0;
        while (pos < Length()) {
            GetCharRange(&ch, pos, 1);
            if (ch == '\r' || ch == '\n')
                break;
            ++pos;
        }
        return pos;
    }
};

void Require(bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void Lex(Scintilla::ILexer5 *lexer, Document &doc, Sci_Position start = 0, Sci_Position length = -1) {
    lexer->Lex(start, length < 0 ? doc.Length() - start : length,
        start ? doc.StyleAt(start - 1) : 0, &doc);
}

void Equal(const Document &a, const Document &b, const std::string &name) {
    Require(a.Length() == b.Length(), name + ": length");
    for (Sci_Position pos = 0; pos < a.Length(); ++pos)
        Require(a.StyleAt(pos) == b.StyleAt(pos), name + ": style at " + std::to_string(pos));
    for (Sci_Position line = 0; line < a.LineFromPosition(a.Length()); ++line)
        Require(a.GetLineState(line) == b.GetLineState(line), name + ": line state " + std::to_string(line));
}

void Expect(Scintilla::ILexer5 *lexer, std::string_view source, std::string_view token, int style, size_t from = 0) {
    Document doc;
    doc.Set(source);
    Lex(lexer, doc);
    const size_t pos = source.find(token, from);
    Require(pos != std::string_view::npos, "Missing test token");
    for (size_t offset = 0; offset < token.size(); ++offset)
        Require(doc.StyleAt(pos + offset) == style, "Wrong style for " + std::string(token) + " at " + std::to_string(offset));
}

void Edited(Scintilla::ILexer5 *lexer, std::string source, size_t pos, std::string_view replacement) {
    Document incremental;
    incremental.Set(source);
    Lex(lexer, incremental);
    source.replace(pos, replacement.size(), replacement);
    // Same-length, same-line-count edits retain the upstream document's
    // styles and line states, reproducing stale state before a partial re-lex.
    incremental.Set(source);
    Lex(lexer, incremental, pos, replacement.size());
    Document full;
    full.Set(source);
    Lex(lexer, full);
    Equal(full, incremental, "edit");
}

} // namespace

int main() {
    const auto release = [](Scintilla::ILexer5 *lexer) { if (lexer) lexer->Release(); };
    std::unique_ptr<Scintilla::ILexer5, decltype(release)> lexer(CreateLexer("markdown"), release);
    Require(lexer != nullptr, "Markdown lexer missing");
    lexer->PropertySet("lexer.markdown.gfm", "1");
    lexer->PropertySet("lexer.markdown.header.eolfill", "1");

    Expect(lexer.get(), "# Heading\n", "# Heading", SCE_MARKDOWN_HEADER1);
    Expect(lexer.get(), "   ###### Heading\n", "###### Heading", SCE_MARKDOWN_HEADER6);
    for (int level = 1; level <= 6; ++level) {
        const std::string title = std::string(level, '#') + " Heading with `code` and $math$";
        Expect(lexer.get(), title + "\n", title, SCE_MARKDOWN_HEADER1 + level - 1);
    }
    Expect(lexer.get(), "#hashtag\n####### invalid\n", "#hashtag", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "####### invalid\n", "####### invalid", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "Title\n=====\n", "Title", SCE_MARKDOWN_HEADER1);
    Expect(lexer.get(), "Title\n-----\n", "Title", SCE_MARKDOWN_HEADER2);
    Expect(lexer.get(), "paragraph\n***\n", "***", SCE_MARKDOWN_HRULE);

    Expect(lexer.get(), "paragraph\n```cpp\n# code\n```\nnormal\n", "# code", SCE_MARKDOWN_CODEBK);
    Expect(lexer.get(), "````\n```\ninside\n````\noutside\n", "inside", SCE_MARKDOWN_CODEBK);
    Expect(lexer.get(), "~~~lang\n```\ninside\n~~~\noutside\n", "inside", SCE_MARKDOWN_CODEBK);
    Expect(lexer.get(), "```\n``` trailing\ninside\n```\noutside\n", "inside", SCE_MARKDOWN_CODEBK);
    Expect(lexer.get(), "```\ntext\n   ````  \noutside\n", "outside", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "```bad`info\nplain\n", "plain", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "```\n\\\n# still code", "# still code", SCE_MARKDOWN_CODEBK);

    Expect(lexer.get(), "- [ ] todo\n", "[ ]", SCE_MARKDOWN_ULIST_ITEM);
    Expect(lexer.get(), "  - [x] done\n", "[x]", SCE_MARKDOWN_ULIST_ITEM);
    Expect(lexer.get(), "> 12) [X] done\n", "[X]", SCE_MARKDOWN_OLIST_ITEM);
    Expect(lexer.get(), "[x] prose\n", "[x]", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "- [y] invalid\n", "[y]", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "1234567890. text\n", "1234567890.", SCE_MARKDOWN_DEFAULT);

    Expect(lexer.get(), "Name | Value\n:--- | ---:\na | b\n", "Name", SCE_MARKDOWN_STRONG1);
    Expect(lexer.get(), "Name | Value\n:--- | ---:\na | b\n", ":--- | ---:", SCE_MARKDOWN_HRULE);
    Expect(lexer.get(), "| Name |\n---\n| value |\n", "value", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "| Name |\n---\n| value |\n", "---", SCE_MARKDOWN_HRULE);
    Expect(lexer.get(), "| Name |\n---\n| value |\n", "|", SCE_MARKDOWN_HRULE, 13);
    Expect(lexer.get(), "Name | Value\n---\n", "Name", SCE_MARKDOWN_HEADER2);
    Expect(lexer.get(), "Name | Value\n--- | bad\n", "Name", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "| A\\|B | C |\n| --- | --- |\n", "A\\|B", SCE_MARKDOWN_STRONG1);
    Expect(lexer.get(), "A | B\n--- | ---\nsolo\na | b\n", "|", SCE_MARKDOWN_HRULE, 21);
    Expect(lexer.get(), "A | B\n--- | ---\n\nplain | text\n", "|", SCE_MARKDOWN_DEFAULT, 20);
    Expect(lexer.get(), "A | B\n    --- | ---\n", "A", SCE_MARKDOWN_DEFAULT);

    Expect(lexer.get(), "word`code`end\n", "`code`", SCE_MARKDOWN_CODE);
    Expect(lexer.get(), "`` a ` b ``\n", "`` a ` b ``", SCE_MARKDOWN_CODE2);
    Expect(lexer.get(), "` unmatched\nplain\n", "plain", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "`$x$ **code**`\n", "`$x$ **code**`", SCE_MARKDOWN_CODE);
    Expect(lexer.get(), "**bold `a*b` text**\n", "`a*b`", SCE_MARKDOWN_CODE);
    Expect(lexer.get(), "**bold `a*b` text**\n", " text**", SCE_MARKDOWN_STRONG1);
    Expect(lexer.get(), "[use `code`](https://example.org/$plain$)\n", "`code`", SCE_MARKDOWN_CODE);
    Expect(lexer.get(), "[use `code`](https://example.org/$plain$)\n", "$plain$", SCE_MARKDOWN_LINK);
    Expect(lexer.get(), "[link](https://example.org/a_(b)) tail\n", "[link](https://example.org/a_(b))", SCE_MARKDOWN_LINK);
    Expect(lexer.get(), "[broken](url\nplain\n", "plain", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "snake_case_word\n", "snake_case_word", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "(**bold**) ~~gone~~\n", "**bold**", SCE_MARKDOWN_STRONG1);
    Expect(lexer.get(), "(**bold**) ~~gone~~\n", "~~gone~~", SCE_MARKDOWN_STRIKEOUT);
    Expect(lexer.get(), "text\n\t\nplain\n", "text", SCE_MARKDOWN_DEFAULT);

    Expect(lexer.get(), "The formula $x_1 + x_2$ is inline.\n", "$x_1 + x_2$", SCE_MARKDOWN_MATH);
    Expect(lexer.get(), "$`x * y + $z`$\n", "$`x * y + $z`$", SCE_MARKDOWN_MATH);
    Expect(lexer.get(), "$100/2$\n", "$100/2$", SCE_MARKDOWN_MATH);
    Expect(lexer.get(), R"($\sqrt{\$4}$)", R"($\sqrt{\$4}$)", SCE_MARKDOWN_MATH);
    Expect(lexer.get(), R"(\$escaped\$)", R"(\$escaped\$)", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "$5 and $10\n", "$5 and $10", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "$ x$ and $y $\n", "$ x$ and $y $", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "$$$invalid$$$\n", "$$$invalid$$$", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "text $unclosed\nplain\n", "plain", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "text $`unclosed\nplain\n", "plain", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "**bold $a*b$ text**\n", "$a*b$", SCE_MARKDOWN_MATH);
    Expect(lexer.get(), "**bold $a*b$ text**\n", " text**", SCE_MARKDOWN_STRONG1);
    Expect(lexer.get(), "[formula $x$](url)\n", "$x$", SCE_MARKDOWN_MATH);
    Expect(lexer.get(), "> - [x] Formula $x$\n", "$x$", SCE_MARKDOWN_MATH);
    Expect(lexer.get(), "A | B\n--- | ---\n$|x|$ | `$y$`\n", "$|x|$", SCE_MARKDOWN_MATH);
    Expect(lexer.get(), "A | B\n--- | ---\n$|x|$ | `$y$`\n", "`$y$`", SCE_MARKDOWN_CODE);
    Expect(lexer.get(), "$$x^2$$\nnormal\n", "$$x^2$$", SCE_MARKDOWN_MATHBK);
    Expect(lexer.get(), "text $$x^2$$ tail\n", "$$x^2$$", SCE_MARKDOWN_MATHBK);
    Expect(lexer.get(), "   $$\nx_1 * x_2\n   $$\nnormal\n", "x_1 * x_2", SCE_MARKDOWN_MATHBK);
    Expect(lexer.get(), "$$\n# math\n$$\nnormal\n", "normal", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "$$x\ny$$ and `code`\n", "`code`", SCE_MARKDOWN_CODE);
    Expect(lexer.get(), "$$\n\\$$\n# still math", "# still math", SCE_MARKDOWN_MATHBK);
    Expect(lexer.get(), "$$\n```cpp\n# still math", "# still math", SCE_MARKDOWN_MATHBK);
    Expect(lexer.get(), "```math\nx_1 * x_2\n```\nnormal\n", "x_1 * x_2", SCE_MARKDOWN_MATHBK);
    Expect(lexer.get(), "```math\nx\n```\nnormal\n", "normal", SCE_MARKDOWN_DEFAULT);
    Expect(lexer.get(), "~~~math\nx\n~~~\n", "x", SCE_MARKDOWN_MATHBK);
    Expect(lexer.get(), "````math\n```\nstill math\n````\n", "still math", SCE_MARKDOWN_MATHBK);
    Expect(lexer.get(), "```mathematica\n$x$\n```\n", "$x$", SCE_MARKDOWN_CODEBK);
    Expect(lexer.get(), "```cpp\n$$\n$x$\n```\n", "$x$", SCE_MARKDOWN_CODEBK);
    Expect(lexer.get(), "```math\n$$\nx", "x", SCE_MARKDOWN_MATHBK);

    const std::string sample = "# Heading\n\n```cpp\n# code\n```\n\nName | Value\n--- | ---\none | two\n\n- [x] done\nTitle\n===\n"
        "\n$$\nx_1 + x_2\n$$\n\n```math\nx * y\n```\n\n**$a*b$ and `x*y`**\n$`x*y`$";
    for (bool crlf : {false, true}) {
        std::string source;
        for (char ch : sample) {
            if (crlf && ch == '\n') source += '\r';
            source += ch;
        }
        Document full;
        full.Set(source);
        Lex(lexer.get(), full);
        Document partial;
        partial.Set(source);
        for (Sci_Position pos = 0; pos < partial.Length(); pos += 7)
            Lex(lexer.get(), partial, pos, std::min<Sci_Position>(7, partial.Length() - pos));
        Equal(full, partial, "chunked lexing");
        for (Sci_Position pos = 0; pos < full.Length(); ++pos) {
            Lex(lexer.get(), partial, pos, 1);
            Equal(full, partial, "restart at byte " + std::to_string(pos));
        }
    }
    Edited(lexer.get(), sample, sample.find("```cpp"), "   ");
    Edited(lexer.get(), sample, sample.find("```\n"), "   ");
    Edited(lexer.get(), sample, sample.find("--- | ---"), "bad");
    Edited(lexer.get(), sample, sample.find("==="), "abc");
    Edited(lexer.get(), "text\nabc\ninside\n```\nafter\n", 5, "```");
    Edited(lexer.get(), "Name | Value\nbad | ---\none | two\n", 13, "---");
    Edited(lexer.get(), sample, sample.find("$$"), "  ");
    Edited(lexer.get(), sample, sample.find("$$\n\n```math"), "  ");
    Edited(lexer.get(), sample, sample.find("math"), "text");
    Edited(lexer.get(), "text\nxx\n# inside\n$$\nafter\n", 5, "$$");
    Edited(lexer.get(), "```text\nx * y\n```\nafter\n", 3, "math");
    Edited(lexer.get(), "$$x\ny$$\nafter\n", 6, "  ");

    // The same lexer instance alternates between documents, just as tabs do.
    Document a, b, expected;
    a.Set("```\ninside\n");
    b.Set("# Other tab\n- [x] item\n");
    expected.Set("# Other tab\n- [x] item\n");
    Lex(lexer.get(), expected);
    Lex(lexer.get(), a, 0, 1);
    Lex(lexer.get(), b, 0, 1);
    Lex(lexer.get(), a, 5, 1);
    Lex(lexer.get(), b, 4, b.Length() - 4);
    Equal(expected, b, "independent documents");
    a.Set("$$\ninside\n");
    Lex(lexer.get(), a, 0, 1);
    Lex(lexer.get(), b, 0, b.Length());
    Equal(expected, b, "independent math documents");
    // The opt-in extension must not alter the upstream lexer path.
    lexer->PropertySet("lexer.markdown.gfm", "0");
    Expect(lexer.get(), "$x$\n", "$x$", SCE_MARKDOWN_DEFAULT);
    std::cout << "Markdown lexer checks passed\n";
}
