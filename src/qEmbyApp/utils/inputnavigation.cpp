#include "inputnavigation.h"
#include "smoothscrollcontroller.h"

#include <QAbstractItemView>
#include <QAbstractButton>
#include <QApplication>
#include <QComboBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPainter>
#include <QPaintEvent>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QTextEdit>
#include <QWidget>
#include <QEasingCurve>
#include <QVariantAnimation>

namespace InputNavigation
{

namespace
{
class FocusPulseOverlay final : public QWidget
{
public:
    explicit FocusPulseOverlay(QWidget* target)
        : QWidget(target ? target->window() : nullptr), m_target(target)
    {
        setObjectName(QStringLiteral("remote-focus-pulse-overlay"));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
        setFocusPolicy(Qt::NoFocus);

        m_animation = new QVariantAnimation(this);
        m_animation->setStartValue(0.0);
        m_animation->setEndValue(1.0);
        m_animation->setDuration(150);
        m_animation->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_animation, &QVariantAnimation::valueChanged, this,
                [this](const QVariant& value) {
                    m_progress = value.toReal();
                    syncGeometry();
                    update();
                });
        connect(m_animation, &QVariantAnimation::finished,
                this, &QObject::deleteLater);
    }

    void start()
    {
        if (!m_target || !parentWidget()) {
            deleteLater();
            return;
        }
        syncGeometry();
        show();
        raise();
        m_animation->start();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        if (!m_target) {
            return;
        }
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QColor color = m_target->palette().color(QPalette::Highlight);
        color.setAlpha(qRound(155.0 * (1.0 - m_progress)));
        painter.setPen(QPen(color, 2.0));
        painter.setBrush(Qt::NoBrush);
        const int expansion = qRound(4.0 * m_progress);
        const int inset = 7 - expansion;
        painter.drawRoundedRect(rect().adjusted(inset, inset, -inset, -inset),
                                8, 8);
    }

private:
    void syncGeometry()
    {
        if (!m_target || !parentWidget()) {
            return;
        }
        constexpr int padding = 8;
        const QPoint topLeft = parentWidget()->mapFromGlobal(
            m_target->mapToGlobal(QPoint(0, 0)));
        setGeometry(QRect(topLeft, m_target->size()).adjusted(
            -padding, -padding, padding, padding));
    }

    QPointer<QWidget> m_target;
    QVariantAnimation* m_animation = nullptr;
    qreal m_progress = 0.0;
};

class ItemFocusPulseOverlay final : public QWidget
{
public:
    ItemFocusPulseOverlay(QAbstractItemView* view, const QModelIndex& index)
        : QWidget(view ? view->viewport() : nullptr),
          m_view(view), m_index(index)
    {
        setObjectName(QStringLiteral("remote-item-focus-pulse-overlay"));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
        setFocusPolicy(Qt::NoFocus);

        m_animation = new QVariantAnimation(this);
        m_animation->setStartValue(0.0);
        m_animation->setEndValue(1.0);
        m_animation->setDuration(150);
        m_animation->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_animation, &QVariantAnimation::valueChanged, this,
                [this](const QVariant& value) {
                    m_progress = value.toReal();
                    update();
                });
        connect(m_animation, &QVariantAnimation::finished,
                this, &QObject::deleteLater);
    }

    void start()
    {
        if (!m_view || !m_index.isValid() || !parentWidget()) {
            deleteLater();
            return;
        }
        setGeometry(parentWidget()->rect());
        show();
        raise();
        m_animation->start();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        if (!m_view || !m_index.isValid()) {
            return;
        }
        const QRect itemRect = m_view->visualRect(m_index);
        if (!itemRect.isValid()) {
            return;
        }
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QColor color = m_view->palette().color(QPalette::Highlight);
        color.setAlpha(qRound(155.0 * (1.0 - m_progress)));
        painter.setPen(QPen(color, 2.0));
        painter.setBrush(Qt::NoBrush);
        const int expansion = qRound(4.0 * m_progress);
        painter.drawRoundedRect(itemRect.adjusted(-expansion, -expansion,
                                                   expansion, expansion),
                                7, 7);
    }

private:
    QPointer<QAbstractItemView> m_view;
    QPersistentModelIndex m_index;
    QVariantAnimation* m_animation = nullptr;
    qreal m_progress = 0.0;
};

void animateFocusedWidget(QWidget* widget)
{
    if (!widget || !widget->window()) {
        return;
    }
    if (auto* previous = widget->window()->findChild<QWidget*>(
            QStringLiteral("remote-focus-pulse-overlay"),
            Qt::FindDirectChildrenOnly)) {
        previous->deleteLater();
    }
    (new FocusPulseOverlay(widget))->start();
}

QRect viewportGlobalRect(QWidget* viewport)
{
    return viewport
        ? QRect(viewport->mapToGlobal(QPoint(0, 0)), viewport->size())
        : QRect();
}

int safeTopMargin(int viewportHeight)
{
    return qBound(24, viewportHeight / 10, 80);
}

int safeBottomMargin(int viewportHeight)
{
    return qBound(64, viewportHeight * 3 / 10, 180);
}

int verticalCorrection(const QRect& target, const QRect& viewport)
{
    if (!target.isValid() || !viewport.isValid()) {
        return 0;
    }

    const int top = viewport.top() + safeTopMargin(viewport.height());
    const int bottom = viewport.bottom() - safeBottomMargin(viewport.height());
    if (bottom <= top || target.height() > bottom - top + 1) {
        return target.center().y() - viewport.center().y();
    }
    if (target.top() < top) {
        return target.top() - top;
    }
    if (target.bottom() > bottom) {
        return target.bottom() - bottom;
    }
    return 0;
}

bool isDirectionalCommand(NavigationCommand command)
{
    return command == NavigationCommand::Left ||
           command == NavigationCommand::Right ||
           command == NavigationCommand::Up ||
           command == NavigationCommand::Down;
}

bool isCandidate(const QRect& source, const QRect& candidate,
                 NavigationCommand command)
{
    switch (command) {
    case NavigationCommand::Left:
        return (source.right() > candidate.right() ||
                source.left() >= candidate.right()) &&
               source.left() > candidate.left();
    case NavigationCommand::Right:
        return (source.left() < candidate.left() ||
                source.right() <= candidate.left()) &&
               source.right() < candidate.right();
    case NavigationCommand::Up:
        return (source.bottom() > candidate.bottom() ||
                source.top() >= candidate.bottom()) &&
               source.top() > candidate.top();
    case NavigationCommand::Down:
        return (source.top() < candidate.top() ||
                source.bottom() <= candidate.top()) &&
               source.bottom() < candidate.bottom();
    default:
        return false;
    }
}

bool beamsOverlap(const QRect& source, const QRect& candidate,
                  NavigationCommand command)
{
    if (command == NavigationCommand::Left ||
        command == NavigationCommand::Right) {
        return candidate.bottom() > source.top() &&
               candidate.top() < source.bottom();
    }
    return candidate.right() > source.left() &&
           candidate.left() < source.right();
}

bool isStrictlyInDirection(const QRect& source, const QRect& candidate,
                           NavigationCommand command)
{
    switch (command) {
    case NavigationCommand::Left: return source.left() >= candidate.right();
    case NavigationCommand::Right: return source.right() <= candidate.left();
    case NavigationCommand::Up: return source.top() >= candidate.bottom();
    case NavigationCommand::Down: return source.bottom() <= candidate.top();
    default: return false;
    }
}

int majorAxisDistance(const QRect& source, const QRect& candidate,
                      NavigationCommand command)
{
    switch (command) {
    case NavigationCommand::Left:
        return qMax(0, source.left() - candidate.right());
    case NavigationCommand::Right:
        return qMax(0, candidate.left() - source.right());
    case NavigationCommand::Up:
        return qMax(0, source.top() - candidate.bottom());
    case NavigationCommand::Down:
        return qMax(0, candidate.top() - source.bottom());
    default:
        return 0;
    }
}

int majorAxisDistanceToFarEdge(const QRect& source, const QRect& candidate,
                               NavigationCommand command)
{
    switch (command) {
    case NavigationCommand::Left:
        return qMax(1, source.left() - candidate.left());
    case NavigationCommand::Right:
        return qMax(1, candidate.right() - source.right());
    case NavigationCommand::Up:
        return qMax(1, source.top() - candidate.top());
    case NavigationCommand::Down:
        return qMax(1, candidate.bottom() - source.bottom());
    default:
        return 1;
    }
}

int minorAxisDistance(const QRect& source, const QRect& candidate,
                      NavigationCommand command)
{
    if (command == NavigationCommand::Left ||
        command == NavigationCommand::Right) {
        return qAbs(source.center().y() - candidate.center().y());
    }
    return qAbs(source.center().x() - candidate.center().x());
}

bool beamBeats(const QRect& source, const QRect& first,
               const QRect& second, NavigationCommand command)
{
    const bool firstInBeam = beamsOverlap(source, first, command);
    const bool secondInBeam = beamsOverlap(source, second, command);
    if (secondInBeam || !firstInBeam) {
        return false;
    }
    if (!isStrictlyInDirection(source, second, command)) {
        return true;
    }
    if (command == NavigationCommand::Left ||
        command == NavigationCommand::Right) {
        return true;
    }
    return majorAxisDistance(source, first, command) <
           majorAxisDistanceToFarEdge(source, second, command);
}

qint64 weightedDistance(const QRect& source, const QRect& candidate,
                        NavigationCommand command)
{
    const qint64 major = majorAxisDistance(source, candidate, command);
    const qint64 minor = minorAxisDistance(source, candidate, command);
    return 13 * major * major + minor * minor;
}

bool isBetterCandidate(const QRect& source, const QRect& candidate,
                       const QRect& currentBest,
                       NavigationCommand command)
{
    if (!isCandidate(source, candidate, command)) {
        return false;
    }
    if (!currentBest.isValid() ||
        !isCandidate(source, currentBest, command)) {
        return true;
    }
    if (beamBeats(source, candidate, currentBest, command)) {
        return true;
    }
    if (beamBeats(source, currentBest, candidate, command)) {
        return false;
    }
    return weightedDistance(source, candidate, command) <
           weightedDistance(source, currentBest, command);
}

void animateScrollBarTo(QScrollBar* bar, int value)
{
    if (!bar) {
        return;
    }

    const int target = qBound(bar->minimum(), value, bar->maximum());
    if (target == bar->value()) {
        return;
    }

    constexpr auto animationName = "remote-focus-scroll-animation";
    auto* animation = bar->findChild<QPropertyAnimation*>(
        animationName, Qt::FindDirectChildrenOnly);
    if (!animation) {
        animation = new QPropertyAnimation(bar, "value", bar);
        animation->setObjectName(animationName);
        animation->setDuration(180);
        animation->setEasingCurve(QEasingCurve::OutCubic);
    }

    animation->stop();
    animation->setStartValue(bar->value());
    animation->setEndValue(target);
    animation->start();
}
} // namespace

std::optional<NavigationCommand> fromKeyEvent(const QKeyEvent* event)
{
    if (!event || event->isAutoRepeat()) {
        return std::nullopt;
    }
    const auto modifiers = event->modifiers() &
        (Qt::ShiftModifier | Qt::ControlModifier |
         Qt::AltModifier | Qt::MetaModifier);
    if (modifiers != Qt::NoModifier) {
        return std::nullopt;
    }

    if (isPlayPauseKey(event->key())) {
        return NavigationCommand::PlayPause;
    }

    switch (event->key()) {
    case Qt::Key_Left: return NavigationCommand::Left;
    case Qt::Key_Right: return NavigationCommand::Right;
    case Qt::Key_Up: return NavigationCommand::Up;
    case Qt::Key_Down: return NavigationCommand::Down;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
    case Qt::Key_Select: return NavigationCommand::Activate;
    case Qt::Key_Back: return NavigationCommand::Back;
    case Qt::Key_Forward: return NavigationCommand::Forward;
    default: return std::nullopt;
    }
}

bool isPlayPauseKey(int key)
{
    return key == Qt::Key_Play || key == Qt::Key_MediaPlay ||
           key == Qt::Key_MediaPause ||
           key == Qt::Key_MediaTogglePlayPause;
}

bool isTextEntryWidget(const QWidget* widget)
{
    return widget &&
        (qobject_cast<const QLineEdit*>(widget) ||
         qobject_cast<const QTextEdit*>(widget) ||
         qobject_cast<const QPlainTextEdit*>(widget) ||
         widget->inherits("QAbstractSpinBox") ||
         qobject_cast<const QComboBox*>(widget));
}

bool moveSpatialFocus(QWidget* scope, NavigationCommand command)
{
    if (!scope || (command != NavigationCommand::Left &&
                   command != NavigationCommand::Right &&
                   command != NavigationCommand::Up &&
                   command != NavigationCommand::Down)) {
        return false;
    }

    QList<QWidget*> candidates;
    const auto descendants = scope->findChildren<QWidget*>();
    for (QWidget* widget : descendants) {
        if (!widget->isVisibleTo(scope) || !widget->isEnabled() ||
            widget->focusPolicy() == Qt::NoFocus ||
            isTextEntryWidget(widget)) {
            continue;
        }
        candidates.append(widget);
    }
    if (candidates.isEmpty()) {
        return false;
    }

    QWidget* current = QApplication::focusWidget();
    if (!current || (current != scope && !scope->isAncestorOf(current))) {
        current = nullptr;
    }
    if (!current) {
        QWidget* first = candidates.first();
        for (QWidget* candidate : candidates) {
            const QPoint a = candidate->mapToGlobal(candidate->rect().center());
            const QPoint b = first->mapToGlobal(first->rect().center());
            if (a.y() < b.y() || (a.y() == b.y() && a.x() < b.x())) {
                first = candidate;
            }
        }
        first->setFocus(Qt::OtherFocusReason);
        animateFocusedWidget(first);
        ensureFocusedWidgetVisible(first);
        return true;
    }

    const QRect sourceRect(current->mapToGlobal(QPoint(0, 0)),
                           current->size());
    QList<QWidget*> spatialCandidates;
    QList<QRect> candidateRects;
    for (QWidget* candidate : candidates) {
        if (candidate == current || current->isAncestorOf(candidate) ||
            candidate->isAncestorOf(current)) {
            continue;
        }
        spatialCandidates.append(candidate);
        candidateRects.append(QRect(
            candidate->mapToGlobal(QPoint(0, 0)), candidate->size()));
    }

    const int bestIndex = bestCandidateIndex(sourceRect, candidateRects,
                                             command);
    if (bestIndex < 0) {
        return false;
    }
    QWidget* best = spatialCandidates.at(bestIndex);
    best->setFocus(Qt::OtherFocusReason);
    animateFocusedWidget(best);
    ensureFocusedWidgetVisible(best);
    return true;
}

bool activateFocusedWidget(QWidget* scope)
{
    QWidget* focused = QApplication::focusWidget();
    if (!focused || !scope ||
        (focused != scope && !scope->isAncestorOf(focused))) {
        return false;
    }
    if (auto* button = qobject_cast<QAbstractButton*>(focused)) {
        if (button->isEnabled()) {
            button->click();
            return true;
        }
    }
    return false;
}

int bestCandidateIndex(const QRect& sourceGlobalRect,
                       const QList<QRect>& candidateGlobalRects,
                       NavigationCommand command)
{
    if (!sourceGlobalRect.isValid() || !isDirectionalCommand(command)) {
        return -1;
    }

    int bestIndex = -1;
    QRect bestRect;
    for (int i = 0; i < candidateGlobalRects.size(); ++i) {
        const QRect candidate = candidateGlobalRects.at(i);
        if (!candidate.isValid() ||
            !isBetterCandidate(sourceGlobalRect, candidate, bestRect,
                               command)) {
            continue;
        }
        bestIndex = i;
        bestRect = candidate;
    }
    return bestIndex;
}

QModelIndex bestItemInDirection(QAbstractItemView* view,
                                const QRect& sourceGlobalRect,
                                NavigationCommand command)
{
    if (!view || !view->viewport() || !view->model() ||
        !sourceGlobalRect.isValid()) {
        return {};
    }

    QList<QModelIndex> indexes;
    QList<QRect> rects;
    const int rowCount = view->model()->rowCount(view->rootIndex());
    for (int row = 0; row < rowCount; ++row) {
        const QModelIndex index = view->model()->index(
            row, 0, view->rootIndex());
        const Qt::ItemFlags flags = index.flags();
        if (!index.isValid() || !flags.testFlag(Qt::ItemIsEnabled) ||
            !flags.testFlag(Qt::ItemIsSelectable)) {
            continue;
        }
        const QRect rect = itemGlobalRect(view, index);
        if (!rect.isValid() || rect == sourceGlobalRect) {
            continue;
        }
        indexes.append(index);
        rects.append(rect);
    }

    const int best = bestCandidateIndex(sourceGlobalRect, rects, command);
    return best >= 0 ? indexes.at(best) : QModelIndex();
}

QRect itemGlobalRect(QAbstractItemView* view, const QModelIndex& index,
                     int padding)
{
    if (!view || !view->viewport() || !index.isValid()) {
        return {};
    }
    const QRect rect = view->visualRect(index);
    if (!rect.isValid()) {
        return {};
    }
    const QPoint topLeft = view->viewport()->mapToGlobal(rect.topLeft());
    return QRect(topLeft, rect.size()).adjusted(-padding, -padding,
                                                padding, padding);
}

bool focusCurrentItem(QAbstractItemView* view,
                      SmoothScrollController* controller)
{
    if (!view || !view->model()) {
        return false;
    }

    const QModelIndex root = view->rootIndex();
    const int rowCount = view->model()->rowCount(root);
    if (rowCount <= 0) {
        return false;
    }

    int row = view->currentIndex().row();
    if (row < 0 || row >= rowCount) {
        row = 0;
    }

    QModelIndex index = view->model()->index(row, 0, root);
    if (!index.isValid() || !index.flags().testFlag(Qt::ItemIsEnabled) ||
        !index.flags().testFlag(Qt::ItemIsSelectable)) {
        index = {};
        for (int candidateRow = 0; candidateRow < rowCount; ++candidateRow) {
            const QModelIndex candidate =
                view->model()->index(candidateRow, 0, root);
            if (candidate.isValid() &&
                candidate.flags().testFlag(Qt::ItemIsEnabled) &&
                candidate.flags().testFlag(Qt::ItemIsSelectable)) {
                index = candidate;
                break;
            }
        }
    }
    if (!index.isValid()) {
        return false;
    }

    view->setCurrentIndex(index);
    view->setFocus(Qt::OtherFocusReason);
    animateItemFocus(view, index);
    ensureItemVisible(view, index, controller);
    return true;
}

bool moveCurrentItem(QAbstractItemView* view, int rowDelta,
                     SmoothScrollController* controller)
{
    if (!view || !view->model() || rowDelta == 0) {
        return false;
    }

    const QModelIndex root = view->rootIndex();
    const int rowCount = view->model()->rowCount(root);
    if (rowCount <= 0) {
        return false;
    }

    const int direction = rowDelta < 0 ? -1 : 1;
    int row = view->currentIndex().row();
    if (row < 0 || row >= rowCount) {
        row = direction > 0 ? -1 : rowCount;
    }

    QModelIndex target;
    for (int candidateRow = row + direction;
         candidateRow >= 0 && candidateRow < rowCount;
         candidateRow += direction) {
        const QModelIndex candidate =
            view->model()->index(candidateRow, 0, root);
        if (candidate.isValid() &&
            candidate.flags().testFlag(Qt::ItemIsEnabled) &&
            candidate.flags().testFlag(Qt::ItemIsSelectable)) {
            target = candidate;
            break;
        }
    }

    if (target.isValid()) {
        view->setCurrentIndex(target);
    }
    return focusCurrentItem(view, controller);
}

void animateWidgetFocus(QWidget* widget)
{
    animateFocusedWidget(widget);
}

void animateItemFocus(QAbstractItemView* view, const QModelIndex& index)
{
    if (!view || !view->viewport() || !index.isValid()) {
        return;
    }
    if (auto* previous = view->viewport()->findChild<QWidget*>(
            QStringLiteral("remote-item-focus-pulse-overlay"),
            Qt::FindDirectChildrenOnly)) {
        previous->deleteLater();
    }
    (new ItemFocusPulseOverlay(view, index))->start();
}

void clearFocusAnimations(QWidget* scope)
{
    if (!scope) {
        return;
    }
    if (QWidget* window = scope->window()) {
        if (auto* overlay = window->findChild<QWidget*>(
                QStringLiteral("remote-focus-pulse-overlay"),
                Qt::FindDirectChildrenOnly)) {
            overlay->hide();
            overlay->deleteLater();
        }
    }
    const auto overlays = scope->findChildren<QWidget*>(
        QStringLiteral("remote-item-focus-pulse-overlay"));
    for (QWidget* overlay : overlays) {
        overlay->hide();
        overlay->deleteLater();
    }
}

bool ensureItemVisible(QAbstractItemView* view, const QModelIndex& index,
                       SmoothScrollController* controller)
{
    if (!view || !view->viewport() || !index.isValid()) {
        return false;
    }

    const QRect target = view->visualRect(index).adjusted(-4, -4, 4, 4);
    const QRect viewport = view->viewport()->rect();
    const int correction = verticalCorrection(target, viewport);
    QScrollBar* bar = view->verticalScrollBar();
    if (!bar || correction == 0) {
        return false;
    }

    const int next = qBound(bar->minimum(), bar->value() + correction,
                            bar->maximum());
    if (next == bar->value()) {
        return false;
    }
    if (controller) {
        controller->scrollTo(next, true);
    } else {
        animateScrollBarTo(bar, next);
    }
    return true;
}

bool ensureGlobalRectVisible(QScrollArea* scrollArea,
                             const QRect& targetGlobalRect,
                             SmoothScrollController* controller)
{
    if (!scrollArea || !scrollArea->viewport() ||
        !targetGlobalRect.isValid()) {
        return false;
    }

    const QRect viewport = viewportGlobalRect(scrollArea->viewport());
    const int correction = verticalCorrection(targetGlobalRect, viewport);
    QScrollBar* bar = scrollArea->verticalScrollBar();
    if (!bar || correction == 0) {
        return false;
    }

    const int current = bar->value();
    const int next = qBound(bar->minimum(), current + correction,
                            bar->maximum());
    if (next == current) {
        return false;
    }

    if (controller) {
        controller->scrollTo(next, true);
    } else {
        animateScrollBarTo(bar, next);
    }
    return true;
}

void ensureFocusedWidgetVisible(QWidget* widget)
{
    if (!widget || !widget->isVisible()) {
        return;
    }
    QWidget* ancestor = widget->parentWidget();
    while (ancestor) {
        if (auto* scrollArea = qobject_cast<QScrollArea*>(ancestor)) {
            // An inner scroll area can move the widget before an outer one is
            // evaluated, so always use its latest global geometry.
            const QRect target(widget->mapToGlobal(QPoint(0, 0)),
                               widget->size());
            ensureGlobalRectVisible(scrollArea, target);
        }
        ancestor = ancestor->parentWidget();
    }
}

} // namespace InputNavigation
