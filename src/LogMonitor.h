/*
 * This file is part of Notepad Next.
 * Copyright 2026 Notepad Next contributors
 *
 * Notepad Next is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef LOGMONITOR_H
#define LOGMONITOR_H

#include <QDateTime>
#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

class ScintillaNext;

// One monitor belongs to one editor, including while its tab is inactive.
class LogMonitor : public QObject
{
    Q_OBJECT

public:
    explicit LogMonitor(ScintillaNext *targetEditor);

    bool isEnabled() const { return enabled; }
    QString statusText() const { return status; }
    bool start();
    void stop();
    void checkNow();

signals:
    void enabledChanged();
    void notice(const QString &message);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    struct FileSnapshot {
        bool exists = false;
        qint64 size = 0;
        QDateTime modified;

        bool operator==(const FileSnapshot &other) const;
    };

    FileSnapshot snapshot() const;
    void updatePath();
    void updateWatchPaths();
    void clearWatchPaths();
    void setStatus(const QString &message);
    void queueUserScrollCheck(int direction = 0);
    qint64 tailFirstVisibleLine() const;
    void scheduleFollow();
    void followTail();

    ScintillaNext *editor;
    QFileSystemWatcher watcher;
    QTimer pollTimer;
    QTimer followTimer;
    QString path;
    QString status;
    FileSnapshot loadedSnapshot;
    bool haveSnapshot = false;
    bool enabled = false;
    bool following = false;
    bool originalReadOnly = false;
    bool forceReload = false;
    bool directoryChanged = false;
    bool loading = false;
    bool userScrollPending = false;
    int userScrollDirection = 0;
    quint64 viewGeneration = 0;
};

#endif // LOGMONITOR_H
