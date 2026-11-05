/*
 * This file is part of Notepad Next.
 * Copyright 2026 Notepad Next contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QBoxLayout>
#include <QStyle>
#include <QVector>
#include <QWidget>

// Keep QBoxLayout's item order and insertion/removal API for ADS. Only the
// geometry changes when wrapping is enabled; the final stretch is left unused.
class WrappingTabLayout : public QBoxLayout
{
public:
    WrappingTabLayout() : QBoxLayout(QBoxLayout::LeftToRight) {}

    bool isWrapping() const { return wrapping; }

    void setWrapping(bool enabled)
    {
        if (wrapping == enabled)
            return;
        wrapping = enabled;
        tabRects.clear();
        invalidate();
    }

    bool hasHeightForWidth() const override
    {
        return wrapping || QBoxLayout::hasHeightForWidth();
    }

    int heightForWidth(int width) const override
    {
        return wrapping ? arrange(QRect(0, 0, width, 0), false)
                        : QBoxLayout::heightForWidth(width);
    }

    int minimumHeightForWidth(int width) const override
    {
        // QBoxLayout's implementation reads a private cache populated by its
        // own heightForWidth(). Wrapped rows bypass that calculation.
        return wrapping ? heightForWidth(width) : QBoxLayout::minimumHeightForWidth(width);
    }

    QSize minimumSize() const override
    {
        if (!wrapping)
            return QBoxLayout::minimumSize();

        QSize size(0, 0);
        for (int i = 0; i < count(); ++i) {
            const auto item = itemAt(i);
            if (!item->widget() || item->isEmpty())
                continue;
            size = size.expandedTo(item->minimumSize());
            size.setHeight(qMax(size.height(), item->sizeHint().height()));
        }
        const auto margins = contentsMargins();
        return size + QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    }

    QSize sizeHint() const override
    {
        return wrapping ? minimumSize() : QBoxLayout::sizeHint();
    }

    void setGeometry(const QRect &rect) override
    {
        if (!wrapping) {
            QBoxLayout::setGeometry(rect);
            return;
        }
        QLayout::setGeometry(rect);
        arrange(rect, true);
    }

    // Positions are in the tabs container, including any vertical scroll offset.
    int tabAt(const QPoint &pos) const
    {
        for (int i = 0; i < tabRects.size(); ++i) {
            if (tabRects.at(i).contains(pos))
                return i;
        }
        return -1;
    }

    int insertionIndexAt(const QPoint &pos) const
    {
        int afterLast = 0;
        const bool rtl = parentWidget() && parentWidget()->isRightToLeft();
        for (int i = 0; i < tabRects.size(); ++i) {
            const auto rect = tabRects.at(i);
            if (rect.isEmpty())
                continue;
            if (pos.y() < rect.top())
                return i;
            if (pos.y() <= rect.bottom()
                && (rtl ? pos.x() > rect.center().x() : pos.x() < rect.center().x()))
                return i;
            afterLast = i + 1;
        }
        return afterLast;
    }

private:
    int arrange(const QRect &rect, bool apply) const
    {
        const auto margins = contentsMargins();
        const QRect area = rect.marginsRemoved(margins);
        const int available = qMax(1, area.width());
        const int gap = qMax(0, spacing());
        int rowHeight = 0;
        for (int i = 0; i < count(); ++i) {
            const auto item = itemAt(i);
            if (item->widget() && !item->isEmpty())
                rowHeight = qMax(rowHeight, item->sizeHint().expandedTo(item->minimumSize()).height());
        }

        if (apply)
            tabRects.fill(QRect(), count());
        int x = area.left();
        int y = area.top();
        bool hasTabs = false;
        const bool rtl = parentWidget() && parentWidget()->isRightToLeft();
        for (int i = 0; i < count(); ++i) {
            auto item = itemAt(i);
            if (!item->widget() || item->isEmpty())
                continue;
            const int width = qMin(available, item->sizeHint().expandedTo(item->minimumSize()).width());
            if (x > area.left() && x + width > area.left() + available) {
                x = area.left();
                y += rowHeight + gap;
            }
            QRect tabRect(x, y, width, rowHeight);
            if (rtl)
                tabRect = QStyle::visualRect(Qt::RightToLeft, area, tabRect);
            if (apply) {
                tabRects[i] = tabRect;
                item->setGeometry(tabRect);
            }
            x += width + gap;
            hasTabs = true;
        }
        return margins.top() + margins.bottom() + (hasTabs ? y - area.top() + rowHeight : 0);
    }

    bool wrapping = false;
    mutable QVector<QRect> tabRects;
};
