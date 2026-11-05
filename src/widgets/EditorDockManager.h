/*
 * This file is part of Notepad Next.
 * Copyright 2026 Notepad Next contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "DockManager.h"

#include <QString>

class EditorDockManager final : public ads::CDockManager
{
public:
    explicit EditorDockManager(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void applyTabStyle();

    QString tabStyleSuffix;
};
