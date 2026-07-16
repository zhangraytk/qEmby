#include "inputnavigation.h"
#include "smoothscrollcontroller.h"
#include "wheelinput.h"

#include <QKeyEvent>
#include <QPointingDevice>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QListWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTest>
#include <QWheelEvent>

namespace
{
QWheelEvent wheelEvent(QPoint pixelDelta, QPoint angleDelta,
                       Qt::ScrollPhase phase = Qt::NoScrollPhase,
                       bool inverted = false)
{
    return QWheelEvent(QPointF(10, 10), QPointF(10, 10), pixelDelta,
                       angleDelta, Qt::NoButton, Qt::NoModifier, phase,
                       inverted, Qt::MouseEventNotSynthesized,
                       QPointingDevice::primaryPointingDevice());
}
}

class InputTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pixelDeltaTakesPriority()
    {
        auto event = wheelEvent(QPoint(17, -9), QPoint(120, 120),
                                Qt::ScrollUpdate);
        QCOMPARE(WheelInput::effectiveDelta(&event), QPoint(17, -9));
        QCOMPARE(WheelInput::delta(&event, Qt::Horizontal), 17);
        QVERIFY(WheelInput::isPrecise(&event));
        QCOMPARE(WheelInput::dominantAxis(&event),
                 WheelInput::Axis::Horizontal);
    }

    void angleDeltaFallbackSupportsFineWheels()
    {
        auto event = wheelEvent({}, QPoint(30, -15));
        QCOMPARE(WheelInput::effectiveDelta(&event), QPoint(30, -15));
        QCOMPARE(WheelInput::delta(&event, Qt::Vertical), -15);
        QCOMPARE(WheelInput::dominantAxis(&event),
                 WheelInput::Axis::Horizontal);

        auto inverted = wheelEvent({}, QPoint(0, 45),
                                   Qt::NoScrollPhase, true);
        QVERIFY(inverted.inverted());
        QCOMPARE(WheelInput::delta(&inverted, Qt::Vertical), 45);

        auto phasedAngle = wheelEvent({}, QPoint(0, 24), Qt::ScrollUpdate);
        QVERIFY(!WheelInput::isPrecise(&phasedAngle));
    }

    void gestureAxisStaysLockedUntilEnd()
    {
        WheelInput::AxisLock lock;
        auto begin = wheelEvent({}, {}, Qt::ScrollBegin);
        QCOMPARE(lock.axisFor(&begin), WheelInput::Axis::None);

        auto horizontal = wheelEvent(QPoint(8, 1), {}, Qt::ScrollUpdate);
        QCOMPARE(lock.axisFor(&horizontal), WheelInput::Axis::Horizontal);

        auto diagonal = wheelEvent(QPoint(1, 12), {}, Qt::ScrollUpdate);
        QCOMPARE(lock.axisFor(&diagonal), WheelInput::Axis::Horizontal);

        auto end = wheelEvent({}, {}, Qt::ScrollEnd);
        QCOMPARE(lock.axisFor(&end), WheelInput::Axis::Horizontal);

        auto vertical = wheelEvent({}, QPoint(0, 120));
        QCOMPARE(lock.axisFor(&vertical), WheelInput::Axis::Vertical);
    }

    void stepAccumulatorSeparatesWheelGestures()
    {
        WheelInput::StepAccumulator accumulator;

        QCOMPARE(accumulator.consume(0.5, Qt::ScrollBegin), 0);
        QCOMPARE(accumulator.consume(0.0, Qt::ScrollEnd), 0);
        QCOMPARE(accumulator.consume(0.5, Qt::ScrollBegin), 0);
        QCOMPARE(accumulator.consume(0.5, Qt::ScrollUpdate), 1);

        QCOMPARE(accumulator.consume(0.75, Qt::ScrollBegin), 0);
        QCOMPARE(accumulator.consume(-0.5, Qt::ScrollUpdate), 0);
        QCOMPARE(accumulator.consume(-0.5, Qt::ScrollEnd), -1);

        QCOMPARE(accumulator.consume(2.4, Qt::NoScrollPhase), 2);
        QCOMPARE(accumulator.consume(0.6, Qt::NoScrollPhase), 1);
        accumulator.reset();
        QCOMPARE(accumulator.consume(0.9, Qt::NoScrollPhase), 0);
    }

    void preciseScrollingIsImmediateAndRespectsBounds()
    {
        QScrollBar bar(Qt::Vertical);
        bar.setRange(0, 200);
        bar.setValue(50);
        SmoothScrollController controller(&bar);

        auto event = wheelEvent(QPoint(0, -15), {}, Qt::ScrollUpdate);
        QVERIFY(controller.scrollByWheelEvent(&event, Qt::Vertical));
        QCOMPARE(bar.value(), 65);
        QCOMPARE(controller.targetValue(), 65);

        auto begin = wheelEvent({}, {}, Qt::ScrollBegin);
        QVERIFY(!controller.scrollByWheelEvent(&begin, Qt::Vertical));
        QCOMPARE(bar.value(), 65);
        auto end = wheelEvent({}, {}, Qt::ScrollEnd);
        QVERIFY(!controller.scrollByWheelEvent(&end, Qt::Vertical));
        QCOMPARE(bar.value(), 65);

        bar.setValue(0);
        controller.stop();
        auto boundary = wheelEvent(QPoint(0, 12), {}, Qt::ScrollUpdate);
        QVERIFY(!controller.scrollByWheelEvent(&boundary, Qt::Vertical));
        QCOMPARE(bar.value(), 0);
    }

    void discreteWheelBuildsAnAnimatedTarget()
    {
        QScrollBar bar(Qt::Vertical);
        bar.setRange(0, 500);
        bar.setValue(100);
        SmoothScrollController controller(&bar);

        auto event = wheelEvent({}, QPoint(0, -30));
        QVERIFY(controller.scrollByWheelEvent(&event, Qt::Vertical));
        QCOMPARE(controller.targetValue(), 130);

        controller.stop();
        bar.setValue(bar.maximum());
        auto boundary = wheelEvent({}, QPoint(0, -120));
        QVERIFY(!controller.scrollByWheelEvent(&boundary, Qt::Vertical));
        QCOMPARE(controller.targetValue(), bar.maximum());
    }

    void standardRemoteKeysMapToCommands()
    {
        QKeyEvent left(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
        QCOMPARE(InputNavigation::fromKeyEvent(&left),
                 std::optional(NavigationCommand::Left));

        QKeyEvent select(QEvent::KeyPress, Qt::Key_Select, Qt::NoModifier);
        QCOMPARE(InputNavigation::fromKeyEvent(&select),
                 std::optional(NavigationCommand::Activate));

        QKeyEvent forward(QEvent::KeyPress, Qt::Key_Forward, Qt::NoModifier);
        QCOMPARE(InputNavigation::fromKeyEvent(&forward),
                 std::optional(NavigationCommand::Forward));

        QKeyEvent mediaPause(QEvent::KeyPress, Qt::Key_MediaPause,
                             Qt::NoModifier);
        QCOMPARE(InputNavigation::fromKeyEvent(&mediaPause),
                 std::optional(NavigationCommand::PlayPause));
        QVERIFY(InputNavigation::isPlayPauseKey(Qt::Key_MediaPause));
        QVERIFY(InputNavigation::isPlayPauseKey(
            Qt::Key_MediaTogglePlayPause));
        QVERIFY(!InputNavigation::isPlayPauseKey(Qt::Key_Stop));

        QKeyEvent modified(QEvent::KeyPress, Qt::Key_Left,
                           Qt::ControlModifier);
        QVERIFY(!InputNavigation::fromKeyEvent(&modified).has_value());
    }

    void directionalFocusUsesVisualGeometryInsteadOfItemOrder()
    {
        const QRect source(410, 100, 120, 80);
        const QList<QRect> nextVisualRow = {
            QRect(20, 240, 120, 80),
            QRect(220, 240, 120, 80),
            QRect(420, 240, 120, 80),
        };

        QCOMPARE(InputNavigation::bestCandidateIndex(
                     source, nextVisualRow, NavigationCommand::Down),
                 2);
        QCOMPARE(InputNavigation::bestCandidateIndex(
                     nextVisualRow.at(2), nextVisualRow,
                     NavigationCommand::Left),
                 1);
    }

    void directionalFocusPrefersSameVisualBeam()
    {
        const QRect source(100, 100, 100, 60);
        const QList<QRect> candidates = {
            QRect(320, 180, 100, 60),
            QRect(105, 200, 100, 60),
        };

        QCOMPARE(InputNavigation::bestCandidateIndex(
                     source, candidates, NavigationCommand::Down),
                 1);
    }

    void itemNavigationUsesLayoutGeometryAndSkipsDisabledItems()
    {
        QListWidget grid;
        grid.resize(320, 260);
        grid.setViewMode(QListView::IconMode);
        grid.setFlow(QListView::LeftToRight);
        grid.setWrapping(true);
        grid.setMovement(QListView::Static);
        grid.setGridSize(QSize(100, 80));
        for (int i = 0; i < 9; ++i) {
            auto* item = new QListWidgetItem(QString::number(i));
            item->setSizeHint(QSize(100, 80));
            grid.addItem(item);
        }
        grid.show();
        QApplication::processEvents();

        const QModelIndex source = grid.model()->index(2, 0);
        const QRect sourceRect = InputNavigation::itemGlobalRect(
            &grid, source);
        QModelIndex target = InputNavigation::bestItemInDirection(
            &grid, sourceRect, NavigationCommand::Down);
        QVERIFY(target.isValid());
        QCOMPARE(target.row(), 5);

        grid.item(5)->setFlags(grid.item(5)->flags() & ~Qt::ItemIsEnabled);
        target = InputNavigation::bestItemInDirection(
            &grid, sourceRect, NavigationCommand::Down);
        QVERIFY(target.isValid());
        QVERIFY(target.row() != 5);
        QVERIFY(target.flags().testFlag(Qt::ItemIsEnabled));
    }

    void linearItemFocusMovesAndClampsAtBoundaries()
    {
        QListWidget list;
        list.addItems({QStringLiteral("First"), QStringLiteral("Disabled"),
                       QStringLiteral("Third")});
        list.item(1)->setFlags(list.item(1)->flags() & ~Qt::ItemIsEnabled);
        list.setCurrentRow(0);
        list.show();
        QApplication::processEvents();

        QVERIFY(InputNavigation::focusCurrentItem(&list));
        QCOMPARE(QApplication::focusWidget(), &list);
        QCOMPARE(list.currentRow(), 0);

        QVERIFY(InputNavigation::moveCurrentItem(&list, 1));
        QCOMPARE(list.currentRow(), 2);
        QVERIFY(InputNavigation::moveCurrentItem(&list, 1));
        QCOMPARE(list.currentRow(), 2);
        QVERIFY(InputNavigation::moveCurrentItem(&list, -1));
        QCOMPARE(list.currentRow(), 0);
    }

    void sideMenuFocusMovesIntoContentAndBackAtItsBoundary()
    {
        QWidget scope;
        scope.resize(500, 240);

        QListWidget menu(&scope);
        menu.setGeometry(10, 10, 180, 220);
        menu.setFocusPolicy(Qt::StrongFocus);
        menu.addItems({QStringLiteral("General"),
                       QStringLiteral("Appearance")});
        menu.setCurrentRow(0);

        QPushButton contentButton(QStringLiteral("Content"), &scope);
        contentButton.setGeometry(280, 80, 140, 48);
        scope.show();
        QApplication::processEvents();

        QVERIFY(InputNavigation::focusCurrentItem(&menu));
        QVERIFY(InputNavigation::moveSpatialFocus(
            &scope, NavigationCommand::Right));
        QCOMPARE(QApplication::focusWidget(), &contentButton);

        QVERIFY(InputNavigation::moveSpatialFocus(
            &scope, NavigationCommand::Left));
        QCOMPARE(QApplication::focusWidget(), &menu);
        QVERIFY(!InputNavigation::moveSpatialFocus(
            &scope, NavigationCommand::Left));
    }

    void spatialFocusSkipsDisabledControlsAndActivatesOnce()
    {
        QWidget scope;
        scope.resize(360, 120);

        QPushButton disabled("disabled", &scope);
        disabled.setGeometry(10, 20, 90, 40);
        disabled.setEnabled(false);

        QPushButton first("first", &scope);
        first.setGeometry(120, 20, 90, 40);
        QPushButton second("second", &scope);
        second.setGeometry(230, 20, 90, 40);
        scope.show();
        QApplication::processEvents();
        if (QWidget* focused = QApplication::focusWidget()) {
            focused->clearFocus();
        }

        QVERIFY(InputNavigation::moveSpatialFocus(
            &scope, NavigationCommand::Right));
        QCOMPARE(QApplication::focusWidget(), &first);

        QVERIFY(InputNavigation::moveSpatialFocus(
            &scope, NavigationCommand::Right));
        QCOMPARE(QApplication::focusWidget(), &second);

        QSignalSpy clicked(&second, &QPushButton::clicked);
        QVERIFY(InputNavigation::activateFocusedWidget(&scope));
        QCOMPARE(clicked.count(), 1);
    }

    void remoteFocusScrollsNestedTargetIntoSafetyZone()
    {
        QScrollArea scroll;
        scroll.resize(320, 400);
        scroll.setWidgetResizable(false);

        auto* content = new QWidget;
        content->resize(300, 1300);
        auto* target = new QPushButton("target", content);
        target->setGeometry(40, 930, 160, 48);
        scroll.setWidget(content);
        scroll.show();
        QApplication::processEvents();

        QCOMPARE(scroll.verticalScrollBar()->value(), 0);
        InputNavigation::ensureFocusedWidgetVisible(target);
        QCOMPARE(scroll.verticalScrollBar()->value(), 0);
        QTRY_VERIFY_WITH_TIMEOUT(scroll.verticalScrollBar()->value() > 0,
                                 250);
        QTRY_VERIFY_WITH_TIMEOUT(
            !scroll.verticalScrollBar()->findChild<QPropertyAnimation*>(
                QStringLiteral("remote-focus-scroll-animation")) ||
            scroll.verticalScrollBar()->findChild<QPropertyAnimation*>(
                QStringLiteral("remote-focus-scroll-animation"))->state() ==
                QAbstractAnimation::Stopped,
            500);

        const QRect viewportGlobal(
            scroll.viewport()->mapToGlobal(QPoint(0, 0)),
            scroll.viewport()->size());
        const QRect targetGlobal(target->mapToGlobal(QPoint(0, 0)),
                                 target->size());
        QVERIFY(viewportGlobal.contains(targetGlobal));
        QVERIFY(targetGlobal.bottom() <= viewportGlobal.bottom() - 64);
    }

    void remoteGridIndexStaysVisibleWhenMovingDown()
    {
        QListWidget list;
        list.resize(320, 260);
        list.setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        for (int i = 0; i < 40; ++i) {
            list.addItem(QStringLiteral("Item %1").arg(i));
        }
        list.show();
        QApplication::processEvents();

        const QModelIndex index = list.model()->index(30, 0);
        QVERIFY(InputNavigation::ensureItemVisible(&list, index));
        QCOMPARE(list.verticalScrollBar()->value(), 0);
        QTRY_VERIFY_WITH_TIMEOUT(list.verticalScrollBar()->value() > 0, 250);
        QTRY_VERIFY_WITH_TIMEOUT(
            list.visualRect(index).bottom() <=
                list.viewport()->rect().bottom() - 64,
            500);
        const QRect rect = list.visualRect(index);
        QVERIFY(list.viewport()->rect().contains(rect));
        QVERIFY(rect.bottom() <= list.viewport()->rect().bottom() - 64);
    }
};

QTEST_MAIN(InputTests)
#include "input_tests.moc"
