/*
 * This file is part of Notepad Next.
 * Copyright 2026 Notepad Next contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "EditorDockManager.h"

#include <QDebug>
#include <QEvent>
#include <QFile>

EditorDockManager::EditorDockManager(QWidget *parent) : ads::CDockManager(parent)
{
    QFile stylesheet(QStringLiteral(":/stylesheets/document-tabs.css"));
    if (stylesheet.open(QIODevice::ReadOnly)) {
        tabStyleSuffix = QStringLiteral("\n") + QString::fromUtf8(stylesheet.readAll());
        applyTabStyle();
    }
    else {
        qWarning() << "Cannot load document tab stylesheet:" << stylesheet.errorString();
    }
}

bool EditorDockManager::eventFilter(QObject *watched, QEvent *event)
{
    // ADS may replace its entire stylesheet when the application palette changes.
    const bool result = ads::CDockManager::eventFilter(watched, event);
    if (event->type() == QEvent::ApplicationPaletteChange)
        applyTabStyle();
    return result;
}

void EditorDockManager::applyTabStyle()
{
    const QString current = styleSheet();
    // Palette notifications can leave the ADS theme unchanged. Do not append
    // duplicate overrides or cause an unnecessary style/layout refresh.
    if (!tabStyleSuffix.isEmpty() && !current.endsWith(tabStyleSuffix))
        setStyleSheet(current + tabStyleSuffix);
}
