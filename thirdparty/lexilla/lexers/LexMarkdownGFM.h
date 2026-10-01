// Markdown highlighting extensions for Notepad Next.
// Distributed under the same license as LexMarkdown.cxx.
// Included inside its anonymous namespace, after the Lexilla headers.
// This is a source highlighter, not a complete CommonMark block parser:
// embedded languages, HTML blocks and list-container continuation are not parsed.

namespace MarkdownGFM {

constexpr int Paragraph = 1;
constexpr int TableHeader = 2;
constexpr int TableBody = 3;
constexpr int DollarMath = 4;
// Fence states are length * 4 + (tilde ? 1 : 0) + (math ? 2 : 0), always >= 12.

bool Space(char ch) noexcept {
    return ch == ' ' || ch == '\t';
}

size_t SkipSpace(std::string_view text, size_t pos = 0) noexcept {
    while (pos < text.size() && Space(text[pos]))
        ++pos;
    return pos;
}

std::string_view Trim(std::string_view text) noexcept {
    text.remove_prefix(SkipSpace(text));
    while (!text.empty() && Space(text.back()))
        text.remove_suffix(1);
    return text;
}

size_t Run(std::string_view text, size_t pos, char ch) noexcept {
    const size_t start = pos;
    while (pos < text.size() && text[pos] == ch)
        ++pos;
    return pos - start;
}

bool Escaped(std::string_view text, size_t pos) noexcept {
    size_t count = 0;
    while (pos && text[--pos] == '\\')
        ++count;
    return count % 2 != 0;
}

void Paint(std::vector<unsigned char> &styles, size_t start, size_t end, int style) {
    std::fill(styles.begin() + start, styles.begin() + end, style);
}

// Count table cells, ignoring escaped pipes (including those in code spans,
// as required by GFM). Leading/trailing pipes do not create extra cells.
size_t Cells(std::string_view text, bool delimiter = false) {
    text = Trim(text);
    if (text.empty())
        return 0;
    if (text.front() == '|')
        text.remove_prefix(1);
    if (!text.empty() && text.back() == '|' && !Escaped(text, text.size() - 1))
        text.remove_suffix(1);
    size_t cells = 0;
    size_t start = 0;
    for (size_t pos = 0; pos <= text.size(); ++pos) {
        if (pos != text.size() && (text[pos] != '|' || Escaped(text, pos)))
            continue;
        auto cell = Trim(text.substr(start, pos - start));
        if (delimiter) {
            if (!cell.empty() && cell.front() == ':')
                cell.remove_prefix(1);
            if (!cell.empty() && cell.back() == ':')
                cell.remove_suffix(1);
            if (cell.empty() || cell.find_first_not_of('-') != std::string_view::npos)
                return 0;
        }
        ++cells;
        start = pos + 1;
    }
    return cells;
}

bool HasPipe(std::string_view text) {
    for (size_t pos = 0; pos < text.size(); ++pos) {
        if (text[pos] == '|' && !Escaped(text, pos))
            return true;
    }
    return false;
}

int Setext(std::string_view text) {
    const size_t indent = text.find_first_not_of(' ');
    if (indent == std::string_view::npos || indent > 3)
        return 0;
    text = Trim(text.substr(indent));
    if (text.empty())
        return 0;
    if ((text.front() == '=' || text.front() == '-') &&
        text.find_first_not_of(text.front()) == std::string_view::npos)
        return text.front() == '=' ? SCE_MARKDOWN_HEADER1 : SCE_MARKDOWN_HEADER2;
    return 0;
}

bool Rule(std::string_view text) {
    text = Trim(text);
    if (text.empty() || (text.front() != '-' && text.front() != '*' && text.front() != '_'))
        return false;
    size_t count = 0;
    for (char ch : text) {
        if (ch == text.front())
            ++count;
        else if (!Space(ch))
            return false;
    }
    return count >= 3;
}

// Returns the position following a balanced pair, or npos. Keep malformed
// links local to their line so they cannot swallow the rest of the document.
size_t Balanced(std::string_view text, size_t start, char open, char close) {
    size_t depth = 0;
    for (size_t pos = start; pos < text.size(); ++pos) {
        if (text[pos] == '\\') {
            ++pos;
        } else if (text[pos] == open) {
            ++depth;
        } else if (text[pos] == close && --depth == 0) {
            return pos + 1;
        }
    }
    return std::string_view::npos;
}

size_t DollarClose(std::string_view text, size_t start) {
    for (size_t pos = start; pos < text.size();) {
        if (text[pos] == '$') {
            const size_t count = Run(text, pos, '$');
            if (count == 2 && !Escaped(text, pos))
                return pos + 2;
            pos += count;
        } else {
            ++pos;
        }
    }
    return std::string_view::npos;
}

// Code and math are opaque to emphasis, links and other Markdown markers.
// Unmatched inline delimiters stay local to their line.
size_t LiteralEnd(std::string_view text, size_t pos, int &style) {
    const char marker = text[pos];
    if (marker != '`' && marker != '$')
        return std::string_view::npos;
    const size_t count = Run(text, pos, marker);
    const size_t start = pos + count;
    if (marker == '$') {
        if (count > 2 || start == text.size())
            return std::string_view::npos;
        style = count == 2 ? SCE_MARKDOWN_MATHBK : SCE_MARKDOWN_MATH;
        if (count == 1 && text[start] == '`') {
            for (size_t end = start + 1; end + 1 < text.size(); ++end) {
                if (text[end] == '`' && text[end + 1] == '$' && !Escaped(text, end))
                    return end + 2;
            }
            return std::string_view::npos;
        }
        // Avoid treating ordinary prices such as "$5 and $10" as math.
        if (count == 1 && Space(text[start]))
            return std::string_view::npos;
    } else {
        style = count == 1 ? SCE_MARKDOWN_CODE : SCE_MARKDOWN_CODE2;
    }
    for (size_t end = start; end < text.size();) {
        if (text[end] != marker) {
            ++end;
            continue;
        }
        const size_t closing = Run(text, end, marker);
        if (closing == count && (marker == '`' || (!Escaped(text, end) &&
            (count == 2 || (end > start && !Space(text[end - 1]) &&
                (end + count == text.size() || !IsADigit(text[end + count])))))))
            return end + count;
        end += closing;
    }
    return std::string_view::npos;
}

void InlineLiterals(std::string_view text, std::vector<unsigned char> &styles, size_t start) {
    for (size_t pos = start; pos < text.size();) {
        if (text[pos] == '\\' && pos + 1 < text.size() &&
            std::ispunct(static_cast<unsigned char>(text[pos + 1]))) {
            pos += 2;
            continue;
        }
        int style = 0;
        const size_t end = LiteralEnd(text, pos, style);
        if (end != std::string_view::npos) {
            Paint(styles, pos, end, style);
            pos = end;
        } else {
            const char marker = text[pos];
            pos += marker == '`' || marker == '$' ? Run(text, pos, marker) : 1;
        }
    }
}

void Inline(std::string_view text, std::vector<unsigned char> &styles, size_t start) {
    for (size_t pos = start; pos < text.size();) {
        const char ch = text[pos];
        if (ch == '\\' && pos + 1 < text.size() &&
            std::ispunct(static_cast<unsigned char>(text[pos + 1]))) {
            pos += 2;
            continue;
        }
        int literalStyle = 0;
        const size_t literalEnd = LiteralEnd(text, pos, literalStyle);
        if (literalEnd != std::string_view::npos) {
            Paint(styles, pos, literalEnd, literalStyle);
            pos = literalEnd;
            continue;
        }
        if (ch == '`' || ch == '$') {
            pos += Run(text, pos, ch);
            continue;
        }
        const size_t bracket = ch == '!' && pos + 1 < text.size() && text[pos + 1] == '[' ? pos + 1 : pos;
        if (text[bracket] == '[') {
            size_t end = Balanced(text, bracket, '[', ']');
            const size_t labelEnd = end;
            if (end != std::string_view::npos && end < text.size()) {
                if (text[end] == '(')
                    end = Balanced(text, end, '(', ')');
                else if (text[end] == '[')
                    end = Balanced(text, end, '[', ']');
                else if (text[end] == ':' && bracket == SkipSpace(text))
                    end = text.size();
                else
                    end = std::string_view::npos;
                if (end != std::string_view::npos) {
                    Paint(styles, pos, end, SCE_MARKDOWN_LINK);
                    InlineLiterals(text.substr(0, labelEnd - 1), styles, bracket + 1);
                    pos = end;
                    continue;
                }
            }
        }
        if (ch == '*' || ch == '_' || ch == '~') {
            const size_t run = Run(text, pos, ch);
            const size_t count = run >= 2 ? 2 : 1;
            const bool wordBefore = pos && (IsAlphaNumeric(static_cast<unsigned char>(text[pos - 1])) ||
                static_cast<unsigned char>(text[pos - 1]) >= 0x80);
            if ((ch != '~' || run == 2) && (ch != '_' || !wordBefore) &&
                pos + run < text.size() && !Space(text[pos + run])) {
                size_t end = pos + run;
                for (; end < text.size(); ++end) {
                    if (text[end] == '\\' && end + 1 < text.size() &&
                        std::ispunct(static_cast<unsigned char>(text[end + 1]))) {
                        ++end;
                        continue;
                    }
                    int innerStyle = 0;
                    const size_t innerEnd = LiteralEnd(text, end, innerStyle);
                    if (innerEnd != std::string_view::npos) {
                        end = innerEnd - 1;
                        continue;
                    }
                    if (text[end] == ch && !Escaped(text, end) && !Space(text[end - 1]) && Run(text, end, ch) >= count) {
                        const size_t after = end + Run(text, end, ch);
                        if (ch == '_' && after < text.size() &&
                            (IsAlphaNumeric(static_cast<unsigned char>(text[after])) || static_cast<unsigned char>(text[after]) >= 0x80))
                            continue;
                        const int style = ch == '~' ? SCE_MARKDOWN_STRIKEOUT :
                            ch == '*' ? (count == 2 ? SCE_MARKDOWN_STRONG1 : SCE_MARKDOWN_EM1) :
                            (count == 2 ? SCE_MARKDOWN_STRONG2 : SCE_MARKDOWN_EM2);
                        Paint(styles, pos, end + count, style);
                        InlineLiterals(text.substr(0, end), styles, pos + run);
                        pos = end + count;
                        break;
                    }
                }
                if (end < text.size())
                    continue;
            }
            pos += run;
            continue;
        }
        ++pos;
    }
}

std::string Line(Accessor &styler, Sci_Position line) {
    return styler.GetRange(styler.LineStart(line), styler.LineEnd(line));
}

// Only top-level headings create sections. Heading styles already exclude
// fenced code and math, while checking the first token excludes containers.
int HeadingLevel(Accessor &styler, Sci_Position line) {
    const std::string text = Line(styler, line);
    const size_t first = SkipSpace(text);
    if (first == text.size() || first > 3 || text.find('\t') < first)
        return 0;
    const int style = styler.StyleAt(styler.LineStart(line) + first);
    if (style < SCE_MARKDOWN_HEADER1 || style > SCE_MARKDOWN_HEADER6)
        return 0;
    // A setext underline belongs to the preceding title, not a new section.
    if (Setext(text) && line > 0 && styler.GetLineState(line - 1) == Paragraph)
        return 0;
    return style - SCE_MARKDOWN_HEADER1 + 1;
}

void FoldHeadings(Sci_PositionU startPos, Sci_Position length, Accessor &styler) {
    if (!styler.GetPropertyInt("fold", 0))
        return;
    const Sci_Position requestedEnd = static_cast<Sci_Position>(startPos) + length;
    const Sci_Position lastLine = styler.GetLine(styler.Length());
    Sci_Position line = styler.GetLine(startPos);
    if (line > 0)
        --line; // The previous heading's fold button may have changed.
    if (line > 0 && styler.GetLineState(line - 1) == Paragraph && Setext(Line(styler, line)))
        --line; // Section content after a setext title starts two lines later.
    int sectionLevel = line > 0 ? FoldLevelStart(styler.LevelAt(line - 1)) : SC_FOLDLEVELBASE;
    sectionLevel = std::max(sectionLevel, SC_FOLDLEVELBASE);

    for (; line <= lastLine; ++line) {
        const int heading = HeadingLevel(styler, line);
        const int level = heading ? SC_FOLDLEVELBASE + heading - 1 : sectionLevel;
        if (heading)
            sectionLevel = level + 1;
        int foldLevel = FoldLevelForCurrentNext(level, sectionLevel);
        if (heading) {
            Sci_Position contentLine = line + 1;
            // An underline alone is not section content; keep empty headings
            // without a button, including at EOF and before a sibling heading.
            if (contentLine <= lastLine && styler.GetLineState(line) == Paragraph &&
                Setext(Line(styler, contentLine)))
                ++contentLine;
            if (contentLine <= lastLine && styler.LineStart(contentLine) < styler.Length()) {
                const int nextHeading = HeadingLevel(styler, contentLine);
                if (!nextHeading || nextHeading > heading)
                    foldLevel |= SC_FOLDLEVELHEADERFLAG;
            }
        }
        const int oldLevel = styler.LevelAt(line);
        styler.SetLevelIfDifferent(line, foldLevel);
        // A changed heading level propagates through its section, then stops
        // when the outgoing level matches the previously calculated context.
        if (styler.LineStart(line + 1) >= requestedEnd && FoldLevelStart(oldLevel) == sectionLevel)
            break;
    }
}

void Colourise(Sci_PositionU startPos, Sci_Position length, Accessor &styler) {
    if (length <= 0)
        return;
    const Sci_Position requestedEnd = static_cast<Sci_Position>(startPos) + length;
    // One line of look-behind is needed when a table delimiter or setext
    // underline is edited. Never keep mutable state in the shared lexer.
    Sci_Position line = styler.GetLine(startPos);
    if (line > 0)
        --line;
    const Sci_Position foldStart = styler.LineStart(line);
    Sci_Position styledEnd = foldStart;
    int state = line > 0 ? styler.GetLineState(line - 1) : 0;
    const Sci_Position lastLine = styler.GetLine(styler.Length());
    const bool fillHeading = styler.GetPropertyInt("lexer.markdown.header.eolfill", 0) != 0;
    styler.StartAt(styler.LineStart(line));
    styler.StartSegment(styler.LineStart(line));

    for (; line <= lastLine; ++line) {
        const Sci_Position lineStart = styler.LineStart(line);
        const Sci_Position nextStart = line < lastLine ? styler.LineStart(line + 1) : styler.Length();
        const std::string text = Line(styler, line);
        const std::string next = line < lastLine ? Line(styler, line + 1) : std::string();
        std::vector<unsigned char> styles(static_cast<size_t>(nextStart - lineStart), SCE_MARKDOWN_DEFAULT);
        const size_t first = SkipSpace(text);
        const bool blockIndent = first <= 3 && text.find('\t', 0) >= first;
        const std::string_view body = std::string_view(text).substr(first);
        const int previous = state;
        state = 0;
        bool block = false;

        if (previous == DollarMath) {
            const size_t end = DollarClose(text, 0);
            Paint(styles, 0, end == std::string_view::npos ? styles.size() : end, SCE_MARKDOWN_MATHBK);
            state = end == std::string_view::npos ? DollarMath : 0;
            if (!state)
                Inline(text, styles, end);
            block = true;
        } else if (previous >= 12) {
            Paint(styles, 0, styles.size(), previous & 2 ? SCE_MARKDOWN_MATHBK : SCE_MARKDOWN_CODEBK);
            const char marker = previous & 1 ? '~' : '`';
            const size_t count = Run(text, first, marker);
            const bool closes = blockIndent && count >= static_cast<size_t>(previous / 4) &&
                SkipSpace(text, first + count) == text.size();
            state = closes ? 0 : previous;
            block = true;
        } else if (blockIndent && Run(text, first, '$') == 2) {
            const size_t end = DollarClose(text, first + 2);
            Paint(styles, first, end == std::string_view::npos ? styles.size() : end, SCE_MARKDOWN_MATHBK);
            state = end == std::string_view::npos ? DollarMath : 0;
            if (!state)
                Inline(text, styles, end);
            block = true;
        } else if (blockIndent && !body.empty() && (body.front() == '`' || body.front() == '~')) {
            const size_t count = Run(text, first, body.front());
            if (count >= 3 && count <= static_cast<size_t>(INT_MAX / 4) &&
                (body.front() == '~' || text.find('`', first + count) == std::string::npos)) {
                const bool math = Trim(std::string_view(text).substr(first + count)) == "math";
                state = static_cast<int>(count * 4) + (body.front() == '~' ? 1 : 0) + (math ? 2 : 0);
                Paint(styles, 0, styles.size(), math ? SCE_MARKDOWN_MATHBK : SCE_MARKDOWN_CODEBK);
                block = true;
            }
        }

        if (!block) {
            size_t content = first;
            // Explicit quote and list prefixes are supported on this line.
            // Their continuation indentation is intentionally not inferred.
            while (content < text.size() && text[content] == '>') {
                Paint(styles, content, text.size(), SCE_MARKDOWN_BLOCKQUOTE);
                content = SkipSpace(text, content + 1);
            }
            const size_t marker = content;
            int listStyle = 0;
            if (content < text.size() && (text[content] == '-' || text[content] == '+' || text[content] == '*') &&
                (content + 1 == text.size() || Space(text[content + 1]))) {
                ++content;
                listStyle = SCE_MARKDOWN_ULIST_ITEM;
            } else {
                size_t digits = content;
                while (digits < text.size() && IsADigit(text[digits]))
                    ++digits;
                if (digits > content && digits - content <= 9 && digits < text.size() &&
                    (text[digits] == '.' || text[digits] == ')') &&
                    (digits + 1 == text.size() || Space(text[digits + 1]))) {
                    content = digits + 1;
                    listStyle = SCE_MARKDOWN_OLIST_ITEM;
                }
            }
            if (listStyle) {
                Paint(styles, marker, content, listStyle);
                content = SkipSpace(text, content);
                if (content + 3 <= text.size() && text[content] == '[' && text[content + 2] == ']' &&
                    (text[content + 1] == ' ' || text[content + 1] == 'x' || text[content + 1] == 'X') &&
                    (content + 3 == text.size() || Space(text[content + 3]))) {
                    Paint(styles, content, content + 3, listStyle);
                    content = SkipSpace(text, content + 3);
                }
            }
            const size_t hashes = Run(text, content, '#');
            const int underline = blockIndent ? Setext(text) : 0;
            const bool plain = content == first && !listStyle && blockIndent && !body.empty();
            if (hashes >= 1 && hashes <= 6 && (content + hashes == text.size() || Space(text[content + hashes])) &&
                (blockIndent || listStyle)) {
                Paint(styles, content, fillHeading ? text.size() : content + hashes,
                    SCE_MARKDOWN_HEADER1 + static_cast<int>(hashes) - 1);
            } else if (underline && previous == Paragraph) {
                Paint(styles, first, text.size(), underline);
            } else if (previous == TableHeader && Cells(text, true)) {
                Paint(styles, first, text.size(), SCE_MARKDOWN_HRULE);
                state = TableBody;
            } else if (blockIndent && Rule(body)) {
                Paint(styles, first, text.size(), SCE_MARKDOWN_HRULE);
            } else {
                const size_t nextIndent = next.find_first_not_of(' ');
                const size_t delimiterCells = nextIndent <= 3 ? Cells(next, true) : 0;
                const bool tableHeader = plain && HasPipe(text) && delimiterCells != 0 && Cells(text) == delimiterCells;
                const bool tableRow = plain && previous == TableBody;
                const int heading = plain && !tableHeader && previous != TableBody ? Setext(next) : 0;
                if (tableHeader) {
                    Paint(styles, first, text.size(), SCE_MARKDOWN_STRONG1);
                    state = TableHeader;
                } else if (heading) {
                    Paint(styles, first, text.size(), heading);
                    state = Paragraph;
                } else {
                    state = tableRow ? TableBody : plain ? Paragraph : 0;
                }
                if (!heading)
                    Inline(text, styles, content);
                if (tableHeader || tableRow) {
                    for (size_t pos = first; pos < text.size(); ++pos) {
                        if (text[pos] == '|' && !Escaped(text, pos) &&
                            styles[pos] != SCE_MARKDOWN_CODE && styles[pos] != SCE_MARKDOWN_CODE2 &&
                            styles[pos] != SCE_MARKDOWN_MATH && styles[pos] != SCE_MARKDOWN_MATHBK)
                            styles[pos] = SCE_MARKDOWN_HRULE;
                    }
                }
            }
        }

        for (size_t pos = 0; pos < styles.size();) {
            size_t end = pos + 1;
            while (end < styles.size() && styles[end] == styles[pos])
                ++end;
            styler.ColourTo(lineStart + end - 1, styles[pos]);
            pos = end;
        }
        const int oldState = styler.GetLineState(line);
        styler.SetLineState(line, state);
        styledEnd = nextStart;
        // Propagate changes past the requested range until the outgoing state
        // converges. This also fixes stale highlighting after deleting a fence.
        if (nextStart >= requestedEnd && state == oldState)
            break;
    }
    styler.Flush();
    // Lex and Fold callbacks receive the same original range. If a fence edit
    // extended colouring beyond that range, folding must cover it as well.
    FoldHeadings(foldStart, styledEnd - foldStart, styler);
}

} // namespace MarkdownGFM
