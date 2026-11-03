/*
 * This file is part of Notepad Next.
 * Copyright 2026 Notepad Next contributors
 *
 * Notepad Next is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "LogMonitor.h"
#include "ScintillaNext.h"

#include <QAbstractSlider>
#include <QEvent>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QWheelEvent>

#include <algorithm>

LogMonitor::LogMonitor(ScintillaNext *targetEditor)
    : QObject(targetEditor), editor(targetEditor), watcher(this), pollTimer(this), followTimer(this)
{
    pollTimer.setInterval(1000);
    connect(&pollTimer, &QTimer::timeout, this, &LogMonitor::checkNow);
    followTimer.setSingleShot(true);
    connect(&followTimer, &QTimer::timeout, this, &LogMonitor::followTail);

    connect(&watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &) {
        // A notification can also reveal an edit with unchanged size and time.
        forceReload = true;
    });
    connect(&watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &) {
        directoryChanged = true;
    });
    connect(editor, &ScintillaNext::closed, this, &LogMonitor::stop);
    connect(editor, &ScintillaNext::saved, this, [this]() {
        if (enabled) {
            updatePath();
            forceReload = true;
        }
    });
    connect(editor, &ScintillaNext::renamed, this, [this]() {
        if (!enabled)
            return;
        if (!editor->isFile()) {
            stop();
            return;
        }
        updatePath();
    });
    connect(editor, &ScintillaNext::reloaded, this, [this]() {
        if (!enabled)
            return;
        // readFromDisk restores the disk's permissions, so reapply the mode.
        editor->setReadOnly(true);
        if (!loading) {
            forceReload = true;
            scheduleFollow();
        }
    });
    connect(editor, &ScintillaEditBase::resized, this, &LogMonitor::scheduleFollow);
    connect(editor, &ScintillaEditBase::zoom, this, [this](int) { scheduleFollow(); });
    connect(editor, &ScintillaEditBase::verticalRangeChanged, this,
            [this](int, int) { scheduleFollow(); });
    connect(editor->verticalScrollBar(), &QAbstractSlider::actionTriggered, this, [this](int action) {
        int direction = 0;
        switch (action) {
        case QAbstractSlider::SliderSingleStepSub:
        case QAbstractSlider::SliderPageStepSub:
        case QAbstractSlider::SliderToMinimum:
            direction = -1;
            break;
        case QAbstractSlider::SliderSingleStepAdd:
        case QAbstractSlider::SliderPageStepAdd:
        case QAbstractSlider::SliderToMaximum:
            direction = 1;
            break;
        case QAbstractSlider::SliderMove: {
            const auto bar = editor->verticalScrollBar();
            direction = (bar->sliderPosition() > bar->value()) - (bar->sliderPosition() < bar->value());
            break;
        }
        default:
            break;
        }
        queueUserScrollCheck(direction);
    });

    editor->installEventFilter(this);
    editor->viewport()->installEventFilter(this);
    editor->verticalScrollBar()->installEventFilter(this);
}

bool LogMonitor::FileSnapshot::operator==(const FileSnapshot &other) const
{
    return exists == other.exists && (!exists || (size == other.size && modified == other.modified));
}

LogMonitor::FileSnapshot LogMonitor::snapshot() const
{
    const QFileInfo info(path);
    if (!info.isFile())
        return {};
    return {true, info.size(), info.lastModified()};
}

bool LogMonitor::start()
{
    if (enabled)
        return true;
    if (!editor->isFile() || editor->canSaveToDisk() || !QFileInfo(editor->getFileInfo().filePath()).isFile())
        return false;

    originalReadOnly = editor->readOnly();
    enabled = true;
    following = true;
    haveSnapshot = false;
    forceReload = true;
    directoryChanged = false;
    ++viewGeneration;
    editor->setReadOnly(true);
    updatePath();
    updateWatchPaths();
    pollTimer.start();
    emit enabledChanged();
    checkNow();
    return true;
}

void LogMonitor::stop()
{
    if (!enabled)
        return;

    enabled = false;
    following = false;
    userScrollPending = false;
    userScrollDirection = 0;
    ++viewGeneration;
    pollTimer.stop();
    followTimer.stop();
    clearWatchPaths();
    setStatus(QString());

    // Respect both the previous editor state and a change in disk permissions.
    const QFileInfo info(path);
    editor->setReadOnly(originalReadOnly || (info.exists() && !info.isWritable()));
    if (editor->isFile() && !info.exists())
        editor->checkFileForStateChange();
    emit enabledChanged();
}

void LogMonitor::clearWatchPaths()
{
    const QStringList files = watcher.files();
    const QStringList directories = watcher.directories();
    if (!files.isEmpty())
        watcher.removePaths(files);
    if (!directories.isEmpty())
        watcher.removePaths(directories);
}

void LogMonitor::updatePath()
{
    if (!editor->isFile())
        return;
    // canonicalFilePath() becomes empty when a log is rotated away.
    const QString newPath = editor->getFileInfo().absoluteFilePath();
    if (newPath == path)
        return;
    clearWatchPaths();
    path = newPath;
    haveSnapshot = false;
    forceReload = true;
    directoryChanged = false;
    updateWatchPaths();
}

void LogMonitor::updateWatchPaths()
{
    if (path.isEmpty())
        return;
    const QFileInfo info(path);
    const QString directory = info.absolutePath();
    if (QFileInfo(directory).isDir() && !watcher.directories().contains(directory))
        watcher.addPath(directory);
    if (info.isFile() && !watcher.files().contains(path))
        watcher.addPath(path);
}

void LogMonitor::setStatus(const QString &message)
{
    if (status == message)
        return;
    status = message;
    if (!message.isEmpty())
        emit notice(message);
}

void LogMonitor::checkNow()
{
    if (!enabled || loading || userScrollPending)
        return;
    if (!editor->isFile()) {
        stop();
        return;
    }

    updatePath();
    const bool lostFileWatch = !watcher.files().contains(path);
    const FileSnapshot before = snapshot();
    updateWatchPaths();
    if (!before.exists) {
        haveSnapshot = false;
        forceReload = true;
        setStatus(tr("Log file is unavailable; waiting for it to reappear."));
        return;
    }

    // Read-only normally prevents edits, but scripts or encoding changes can
    // still make the buffer dirty. Never discard such changes automatically.
    if (editor->canSaveToDisk()) {
        editor->setReadOnly(true);
        setStatus(tr("Log monitoring paused: turn it off and save the document's changes."));
        return;
    }

    const bool shouldReload = forceReload || !haveSnapshot || !(before == loadedSnapshot)
        || (directoryChanged && lostFileWatch);
    directoryChanged = false;
    if (!shouldReload) {
        setStatus(QString());
        return;
    }

    const auto topLine = editor->firstVisibleLine();
    loading = true;
    ++viewGeneration;
    const bool success = editor->reloadWithEncoding(editor->savedEncoding());
    editor->setReadOnly(true);
    loading = false;
    if (!success) {
        forceReload = true;
        setStatus(tr("Cannot read the log file; retrying: %1").arg(editor->fileErrorString()));
        return;
    }

    // If the file changed while it was being read, the next poll must read it
    // again rather than recording the newer metadata as already displayed.
    loadedSnapshot = before;
    haveSnapshot = true;
    forceReload = !(snapshot() == before);
    setStatus(QString());
    if (following)
        scheduleFollow();
    else
        editor->setFirstVisibleLine(topLine);
}

bool LogMonitor::eventFilter(QObject *watched, QEvent *event)
{
    if (!enabled)
        return QObject::eventFilter(watched, event);

    switch (event->type()) {
    case QEvent::Show:
        if (watched == editor)
            scheduleFollow();
        break;
    case QEvent::Wheel: {
        const auto wheel = static_cast<QWheelEvent *>(event);
        if (!(wheel->modifiers() & Qt::ControlModifier)) {
            const int delta = wheel->angleDelta().y() != 0 ? wheel->angleDelta().y() : wheel->pixelDelta().y();
            queueUserScrollCheck((delta < 0) - (delta > 0));
        }
        break;
    }
    case QEvent::KeyPress: {
        const int key = static_cast<QKeyEvent *>(event)->key();
        int direction = 0;
        if (key == Qt::Key_Up || key == Qt::Key_PageUp || key == Qt::Key_Home)
            direction = -1;
        else if (key == Qt::Key_Down || key == Qt::Key_PageDown || key == Qt::Key_End)
            direction = 1;
        queueUserScrollCheck(direction);
        break;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
        queueUserScrollCheck();
        break;
    case QEvent::MouseMove:
        if (static_cast<QMouseEvent *>(event)->buttons() & Qt::LeftButton)
            queueUserScrollCheck();
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

void LogMonitor::queueUserScrollCheck(int direction)
{
    if (!enabled || loading)
        return;
    if (userScrollPending) {
        if (direction != 0)
            userScrollDirection = direction;
        return;
    }
    userScrollPending = true;
    userScrollDirection = direction;
    const auto before = editor->firstVisibleLine();
    const quint64 generation = viewGeneration;
    QTimer::singleShot(0, this, [this, before, generation]() {
        if (!enabled || generation != viewGeneration)
            return;
        userScrollPending = false;
        const int direction = userScrollDirection;
        userScrollDirection = 0;
        if (loading)
            return;
        const auto after = editor->firstVisibleLine();
        if (after < before && following) {
            following = false;
            followTimer.stop();
            emit notice(tr("Log following paused. Scroll back to the end to resume."));
        }
        else if (!following && (after > before || (after == before && direction > 0)) &&
                 after >= std::min<qint64>(tailFirstVisibleLine(), editor->verticalScrollBar()->maximum())) {
            following = true;
            emit notice(tr("Log following resumed."));
            scheduleFollow();
        }
    });
}

qint64 LogMonitor::tailFirstVisibleLine() const
{
    const auto lastLine = std::max<sptr_t>(0, editor->lineCount() - 1);
    const auto lastDisplayLine = editor->visibleFromDocLine(lastLine)
        + std::max<sptr_t>(1, editor->wrapCount(lastLine)) - 1;
    const auto screenLines = std::max<sptr_t>(1, editor->linesOnScreen());
    return std::max<qint64>(0, lastDisplayLine - (3 * (screenLines - 1) / 4));
}

void LogMonitor::scheduleFollow()
{
    if (enabled && following && !loading && !followTimer.isActive())
        followTimer.start(0);
}

void LogMonitor::followTail()
{
    if (!enabled || !following || loading || !editor->isVisible())
        return;
    if (userScrollPending) {
        followTimer.start(10);
        return;
    }

    const auto lastLine = std::max<sptr_t>(0, editor->lineCount() - 1);
    editor->ensureVisible(lastLine);
    const auto target = tailFirstVisibleLine();
    if (editor->firstVisibleLine() != target) {
        ++viewGeneration;
        editor->setFirstVisibleLine(target);
    }
}
