#include "slidingstackedwidget.h"

#include <config/config_keys.h>
#include <config/configstore.h>

#include <QPointer>
#include <QStandardPaths>
#include <QTest>
#include <QWidget>

class SlidingStackedWidgetTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        ConfigStore::instance()->set(ConfigKeys::UiAnimations, false);
    }

    void finishingTransitionCommitsTargetBeforeDisposal()
    {
        SlidingStackedWidget stack;
        stack.resize(640, 360);
        stack.setSpeed(1000);

        auto *first = new QWidget;
        auto *second = new QWidget;
        QPointer<QWidget> firstGuard(first);
        stack.addWidget(first);
        stack.addWidget(second);
        stack.setCurrentWidget(first);
        stack.show();
        QApplication::processEvents();

        stack.slideInWgt(second);
        QCOMPARE(stack.currentWidget(), first);
        stack.disposeWidgetWhenSafe(first);
        stack.finishActiveTransition();

        QCOMPARE(stack.currentWidget(), second);
        QCOMPARE(stack.indexOf(first), -1);
        QTRY_VERIFY(firstGuard.isNull());
    }

    void interruptedTransitionDropsADisposedTargetSafely()
    {
        SlidingStackedWidget stack;
        stack.resize(640, 360);
        stack.setSpeed(1000);

        auto *first = new QWidget;
        auto *second = new QWidget;
        QPointer<QWidget> secondGuard(second);
        stack.addWidget(first);
        stack.addWidget(second);
        stack.setCurrentWidget(first);
        stack.show();
        QApplication::processEvents();

        stack.slideInWgt(second);
        stack.disposeWidgetWhenSafe(second);
        stack.slideInWgt(second);

        QCOMPARE(stack.count(), 1);
        QCOMPARE(stack.currentWidget(), first);
        QTRY_VERIFY(secondGuard.isNull());
    }
};

QTEST_MAIN(SlidingStackedWidgetTests)
#include "slidingstackedwidget_tests.moc"
