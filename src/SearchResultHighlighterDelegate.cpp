/*
 * This file is part of Notepad Next.
 * Copyright 2025 Justin Dailey
 *
 * Notepad Next is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Notepad Next is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Notepad Next.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "SearchResultHighlighterDelegate.h"
#include "SearchResultData.h"
#include "Theme.h"

#include <QPainter>

void SearchResultHighlighterDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Only custom paint for column 1 with highlight flag set
    if (index.column() != 1 || !index.data(SearchResultData::LineNumber).isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    const QString text = index.data(Qt::DisplayRole).toString();
    int start = index.data(SearchResultData::LinePosStart).toInt();
    int end = index.data(SearchResultData::LinePosEnd).toInt();

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    painter->save();
    painter->setClipRect(opt.rect);

    // Optional: draw background or selection as usual
    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, opt.palette.highlight());
    } else {
        painter->fillRect(opt.rect, opt.backgroundBrush);
    }

    QRect textRect = opt.rect.adjusted(4, 0, -4, 0); // small padding
    QFontMetrics fm(opt.font);
    int y = textRect.top() + (textRect.height() + fm.ascent() - fm.descent()) / 2;
    int x = textRect.left();

    // Split the text
    const QString before = text.left(start);
    const QString match = text.mid(start, end - start);
    const QString after = text.mid(end);

    // Draw 'before' text (normal)
    painter->setFont(opt.font);
    const auto textRole = opt.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text;
    painter->setPen(opt.palette.color(textRole));
    painter->drawText(x, y, before);
    x += fm.horizontalAdvance(before);

    // Keep the match distinct on both light and dark backgrounds.
    QFont boldFont = opt.font;
    boldFont.setBold(true);
    painter->setFont(boldFont);

    int matchWidth = fm.horizontalAdvance(match);
    QRect highlightRect(x, textRect.top(), matchWidth, textRect.height());
    const bool dark = opt.palette.color(QPalette::Base).lightness() < 128;
    painter->fillRect(highlightRect, dark ? QColor(Theme::Dark::SearchBackground) : QColor(Qt::yellow));

    painter->setPen(dark ? QColor(Theme::Dark::SearchText) : QColor(Qt::red));
    painter->drawText(x, y, match);
    x += matchWidth;

    // Draw 'after' text (normal)
    painter->setFont(opt.font);
    painter->setPen(opt.palette.color(textRole));
    painter->drawText(x, y, after);

    painter->restore();
}
