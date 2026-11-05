/*
 * This file is part of Notepad Next.
 * Copyright 2026 Notepad Next contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ApplicationSettings.h"
#include "WrappingTabLayout.h"
#include "DockAreaTabBar.h"
#include "DockAreaTitleBar.h"
#include "DockAreaWidget.h"
#include "DockManager.h"
#include "DockWidget.h"
#include "DockWidgetTab.h"
#include "ElidingLabel.h"

#include <QAbstractButton>
#include <QFile>
#include <QMainWindow>
#include <QMouseEvent>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class HintedTab : public QWidget
{
public:
    QSize sizeHint() const override { return QSize(100, 24); }
    QSize minimumSizeHint() const override { return QSize(20, 24); }
};

class TabsWindow : public QMainWindow
{
public:
    ads::CDockManager *manager;

    TabsWindow()
    {
        manager = new ads::CDockManager(this);
        connect(manager, &ads::CDockManager::dockAreaCreated, this, [](ads::CDockAreaWidget *area) {
            area->titleBar()->tabBar()->setTabsWrapping(true);
        });
        resize(600, 700);
    }

    ads::CDockAreaWidget *addDocuments(int count, ads::CDockAreaWidget *area = nullptr)
    {
        for (int i = 0; i < count; ++i) {
            auto dock = manager->createDockWidget(QStringLiteral("document-%1.txt").arg(i));
            auto content = new QWidget;
            content->setMinimumSize(20, 20);
            dock->setWidget(content);
            dock->setFeature(ads::CDockWidget::DockWidgetFloatable, false);
            area = manager->addDockWidget(ads::CenterDockWidgetArea, dock, area);
        }
        return area;
    }
};

class WrappedTabsTest : public QObject
{
    Q_OBJECT

    // Active tabs have different QSS margins. Compare vertical spans rather
    // than requiring identical widget y coordinates within a visual row.
    static bool sameRow(const QWidget *first, const QWidget *second)
    {
        const auto a = first->geometry();
        const auto b = second->geometry();
        return qMax(a.top(), b.top()) <= qMin(a.bottom(), b.bottom());
    }

    static bool centerInViewport(ads::CDockAreaTabBar *bar, const QWidget *tab)
    {
        return bar->viewport()->rect().contains(tab->mapTo(bar->viewport(), tab->rect().center()));
    }

    static void sendMouseEvent(QWidget *widget, QEvent::Type type, const QPoint &globalPos,
        Qt::MouseButton button, Qt::MouseButtons buttons)
    {
        QMouseEvent event(type, QPointF(widget->mapFromGlobal(globalPos)),
            QPointF(widget->window()->mapFromGlobal(globalPos)), QPointF(globalPos),
            button, buttons, Qt::NoModifier);
        QApplication::sendEvent(widget, &event);
    }

private slots:
    void initTestCase()
    {
        QVERIFY(settingsDirectory.isValid());
        QCoreApplication::setOrganizationName(QStringLiteral("NotepadNextTests"));
        QCoreApplication::setApplicationName(QStringLiteral("WrappedTabsTest"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, settingsDirectory.path());
        QApplication::setStyle(QStringLiteral("Fusion"));
        QFile stylesheet(QStringLiteral(NOTEPADNEXT_TEST_STYLESHEET));
        QVERIFY(stylesheet.open(QIODevice::ReadOnly));
        qApp->setStyleSheet(QString::fromUtf8(stylesheet.readAll()));
        ads::CDockManager::setConfigFlag(ads::CDockManager::AllTabsHaveCloseButton, true);
        ads::CDockManager::setConfigFlag(ads::CDockManager::AlwaysShowTabs, true);
        ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
        ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasCloseButton, false);
        ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasUndockButton, false);
        ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasTabsMenuButton, false);
        ads::CDockManager::setConfigFlag(ads::CDockManager::MiddleMouseButtonClosesTab, true);
    }

    void settingDefaultsToWrappingAndPersists()
    {
        {
            ApplicationSettings settings;
            QVERIFY(settings.wrapTabs());
            QSignalSpy changed(&settings, &ApplicationSettings::wrapTabsChanged);
            settings.setWrapTabs(false);
            QCOMPARE(changed.count(), 1);
            settings.sync();
        }
        ApplicationSettings restored;
        QVERIFY(!restored.wrapTabs());
        restored.clear();
    }

    void layoutWrapsWithoutChangingLogicalOrder()
    {
        QWidget container;
        auto layout = new WrappingTabLayout;
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        container.setLayout(layout);
        QVector<HintedTab *> tabs;
        for (int i = 0; i < 4; ++i) {
            auto tab = new HintedTab;
            tabs.append(tab);
            layout->addWidget(tab);
        }
        layout->addStretch();
        layout->setWrapping(true);
        container.show();
        layout->setGeometry(QRect(0, 0, 200, 48));
        QCOMPARE(tabs[0]->geometry(), QRect(0, 0, 100, 24));
        QCOMPARE(tabs[1]->geometry(), QRect(100, 0, 100, 24));
        QCOMPARE(tabs[2]->geometry(), QRect(0, 24, 100, 24));
        QCOMPARE(layout->heightForWidth(200), 48);
        QCOMPARE(layout->minimumHeightForWidth(200), 48);
        QCOMPARE(layout->minimumHeightForWidth(100), 96);
        QCOMPARE(layout->minimumHeightForWidth(400), 24);
        QCOMPARE(layout->minimumHeightForWidth(200), 48);
        QCOMPARE(layout->tabAt(QPoint(50, 36)), 2);
        QCOMPARE(layout->insertionIndexAt(QPoint(5, 36)), 2);
        QCOMPARE(layout->insertionIndexAt(QPoint(195, 36)), 4);

        tabs[1]->hide();
        layout->setGeometry(QRect(0, 0, 200, 48));
        QCOMPARE(tabs[2]->geometry(), QRect(100, 0, 100, 24));
        QCOMPARE(layout->indexOf(tabs[2]), 2);
        layout->setGeometry(QRect(0, 0, 400, 24));
        QCOMPARE(layout->heightForWidth(400), 24);
        QCOMPARE(layout->minimumHeightForWidth(400), 24);
        QCOMPARE(tabs[3]->y(), 0);
    }

    void tabsReflowOnResizeAndClose()
    {
        TabsWindow window;
        auto area = window.addDocuments(8);
        auto bar = area->titleBar()->tabBar();
        window.show();
        QTRY_VERIFY(!sameRow(bar->tab(7), bar->tab(0)));
        QTRY_VERIFY(bar->width() >= area->width() - 4);
        QTRY_COMPARE(bar->height(), bar->heightForWidth(bar->width()));
        QTRY_COMPARE(bar->verticalScrollBar()->maximum(), 0);
        QTRY_VERIFY(area->titleBar()->height() >= bar->height());
        QCOMPARE(bar->horizontalScrollBar()->maximum(), 0);
        const int wrappedHeight = bar->height();
        const int wrappedTitleBarHeight = area->titleBar()->height();

        QTest::mouseClick(bar->tab(0), Qt::LeftButton);
        QTRY_COMPARE(area->currentIndex(), 0);
        auto closing = area->dockWidget(7);
        auto button = bar->tab(7)->findChild<QAbstractButton *>(QStringLiteral("tabCloseButton"));
        QVERIFY(button);
        QTest::mouseClick(button, Qt::LeftButton);
        QTRY_VERIFY(closing->isClosed());

        window.resize(2000, 700);
        QTRY_VERIFY(sameRow(bar->tab(6), bar->tab(0)));
        QTRY_VERIFY(bar->height() < wrappedHeight);
        QTRY_VERIFY(area->titleBar()->height() < wrappedTitleBarHeight);
        window.resize(450, 700);
        QTRY_VERIFY(!sameRow(bar->tab(6), bar->tab(0)));
        QTRY_COMPARE(bar->height(), bar->heightForWidth(bar->width()));
        const int narrowHeight = bar->height();
        bar->setTabsWrapping(false);
        QTRY_VERIFY(sameRow(bar->tab(6), bar->tab(0)));
        QTRY_VERIFY(bar->height() < narrowHeight);
        bar->setTabsWrapping(true);
        QTRY_VERIFY(!sameRow(bar->tab(6), bar->tab(0)));
        QTRY_COMPARE(bar->height(), bar->heightForWidth(bar->width()));
    }

    void draggingAcrossRowsReordersTheDockWidgets()
    {
        TabsWindow window;
        auto area = window.addDocuments(9);
        auto bar = area->titleBar()->tabBar();
        window.show();
        QTRY_VERIFY(!sameRow(bar->tab(8), bar->tab(0)));
        auto movingDock = area->dockWidget(0);
        auto movingTab = bar->tab(0);
        auto target = bar->tab(8);
        // Finish activation, automatic scrolling and height updates before
        // taking coordinates. Geometry alone does not imply viewport visibility.
        area->setCurrentIndex(0);
        QTRY_COMPARE(area->currentIndex(), 0);
        QTRY_COMPARE(bar->height(), bar->heightForWidth(bar->width()));
        QTRY_COMPARE(bar->verticalScrollBar()->maximum(), 0);
        QTRY_VERIFY(centerInViewport(bar, movingTab));
        QTRY_VERIFY(centerInViewport(bar, target));
        QSignalSpy moved(bar, &ads::CDockAreaTabBar::tabMoved);
        QSignalSpy dragFinished(movingTab, &ads::CDockWidgetTab::moved);
        const QPoint start = movingTab->mapToGlobal(movingTab->rect().center());
        sendMouseEvent(movingTab, QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        QCOMPARE(movingTab->dragState(), ads::DraggingMousePressed);
        const QPoint destination = target->mapToGlobal(QPoint(target->width() - 4, target->height() / 2));
        QVERIFY(bar->viewport()->rect().contains(bar->viewport()->mapFromGlobal(destination)));
        QVERIFY((destination - start).manhattanLength() >= QApplication::startDragDistance());
        QCOMPARE(bar->tabInsertIndexAt(bar->mapFromGlobal(destination)), 9);
        // Send every event with consistent local, window and global positions,
        // independent of the offscreen platform's native mouse handling.
        for (int i = 0; i < 2; ++i) {
            sendMouseEvent(movingTab, QEvent::MouseMove, destination, Qt::NoButton, Qt::LeftButton);
            QCOMPARE(movingTab->dragState(), ads::DraggingTab);
        }
        sendMouseEvent(movingTab, QEvent::MouseButtonRelease, destination, Qt::LeftButton, Qt::NoButton);
        QCOMPARE(dragFinished.count(), 1);
        QTRY_COMPARE(moved.count(), 1);
        QCOMPARE(area->dockWidget(8), movingDock);
        QCOMPARE(bar->tab(8), movingTab);
        QCOMPARE(area->currentIndex(), 8);
    }

    void longTitlesAndFontChangesReflow()
    {
        TabsWindow window;
        auto area = window.addDocuments(6);
        auto bar = area->titleBar()->tabBar();
        window.show();
        QTRY_VERIFY(!sameRow(bar->tab(5), bar->tab(0)));
        auto tab = bar->tab(1);
        const QString fullPath = QStringLiteral("/workspace/project/document-1.txt");
        tab->setToolTip(fullPath);
        area->dockWidget(1)->setWindowTitle(QString(300, QLatin1Char('W')));
        QTRY_VERIFY(tab->width() <= bar->viewport()->width());
        QTRY_VERIFY(tab->width() >= bar->viewport()->width() - 4);
        QTRY_VERIFY(tab->isTitleElided());
        auto titleLabel = tab->findChild<ads::CElidingLabel *>();
        QVERIFY(titleLabel);
        QCOMPARE(titleLabel->toolTip(), fullPath);
        const int oldRowHeight = bar->tab(0)->height();
        for (int i = 0; i < bar->count(); ++i) {
            auto label = bar->tab(i)->findChild<ads::CElidingLabel *>();
            QVERIFY(label);
            QFont font = label->font();
            font.setPointSize(font.pointSize() + 8);
            label->setFont(font);
            label->updateGeometry();
            bar->tab(i)->updateGeometry();
        }
        QTRY_VERIFY(bar->tab(0)->height() > oldRowHeight);
        QCOMPARE(bar->horizontalScrollBar()->maximum(), 0);
    }

    void manyRowsScrollAndKeepTheCurrentTabVisible()
    {
        TabsWindow window;
        auto area = window.addDocuments(60);
        auto bar = area->titleBar()->tabBar();
        window.show();
        QTRY_VERIFY(bar->verticalScrollBar()->maximum() > 0);
        QVERIFY(bar->height() <= area->height() / 2);
        area->setCurrentIndex(0);
        QTRY_COMPARE(bar->verticalScrollBar()->value(), 0);
        area->setCurrentIndex(59);
        auto last = bar->tab(59);
        QTRY_VERIFY(bar->viewport()->rect().contains(last->mapTo(bar->viewport(), last->rect().center())));
        const int previousRange = bar->verticalScrollBar()->maximum();
        for (auto dock : area->dockWidgets())
            dock->setWindowTitle(QString(300, QLatin1Char('W')));
        QTRY_VERIFY(bar->verticalScrollBar()->maximum() > previousRange);
        QTRY_VERIFY(bar->viewport()->rect().contains(last->mapTo(bar->viewport(), last->rect().center())));
        const QPoint leftHalf = bar->mapFromGlobal(last->mapToGlobal(QPoint(4, last->height() / 2)));
        QCOMPARE(bar->tabInsertIndexAt(leftHalf), 59);
    }

    void splitAreasWrapIndependently()
    {
        TabsWindow window;
        auto left = window.addDocuments(6);
        auto dock = window.manager->createDockWidget(QStringLiteral("right.txt"));
        dock->setWidget(new QWidget);
        auto right = window.manager->addDockWidget(ads::RightDockWidgetArea, dock, left);
        window.addDocuments(5, right);
        window.resize(900, 700);
        window.show();
        QVERIFY(left->titleBar()->tabBar()->tabsWrapping());
        QVERIFY(right->titleBar()->tabBar()->tabsWrapping());
        QTRY_VERIFY(!sameRow(left->titleBar()->tabBar()->tab(5), left->titleBar()->tabBar()->tab(0)));
        QTRY_VERIFY(!sameRow(right->titleBar()->tabBar()->tab(5), right->titleBar()->tabBar()->tab(0)));
        right->titleBar()->tabBar()->setTabsWrapping(false);
        QTRY_VERIFY(sameRow(right->titleBar()->tabBar()->tab(5), right->titleBar()->tabBar()->tab(0)));
        QVERIFY(!sameRow(left->titleBar()->tabBar()->tab(5), left->titleBar()->tabBar()->tab(0)));
    }

private:
    QTemporaryDir settingsDirectory;
};

QTEST_MAIN(WrappedTabsTest)
#include "WrappedTabsTest.moc"
