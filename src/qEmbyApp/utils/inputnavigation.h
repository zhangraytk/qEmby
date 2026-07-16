#ifndef INPUTNAVIGATION_H
#define INPUTNAVIGATION_H

#include <QRect>
#include <QList>
#include <optional>

class QAbstractItemView;
class QKeyEvent;
class QModelIndex;
class QScrollArea;
class QWidget;
class SmoothScrollController;

enum class InputMode
{
    Pointer,
    RemoteFocus,
};

enum class NavigationCommand
{
    Left,
    Right,
    Up,
    Down,
    Activate,
    Back,
    Forward,
    PlayPause,
};

namespace InputNavigation
{
std::optional<NavigationCommand> fromKeyEvent(const QKeyEvent* event);
bool isPlayPauseKey(int key);
bool isDirectionalKey(int key);
bool isTextEntryWidget(const QWidget* widget);
bool moveSpatialFocus(QWidget* scope, NavigationCommand command);
bool activateFocusedWidget(QWidget* scope);
int bestCandidateIndex(const QRect& sourceGlobalRect,
                       const QList<QRect>& candidateGlobalRects,
                       NavigationCommand command);
QModelIndex bestItemInDirection(QAbstractItemView* view,
                                const QRect& sourceGlobalRect,
                                NavigationCommand command);
QRect itemGlobalRect(QAbstractItemView* view, const QModelIndex& index,
                     int padding = 0);
bool focusCurrentItem(QAbstractItemView* view,
                      SmoothScrollController* controller = nullptr);
bool moveCurrentItem(QAbstractItemView* view, int rowDelta,
                     SmoothScrollController* controller = nullptr);
void animateWidgetFocus(QWidget* widget);
void animateItemFocus(QAbstractItemView* view, const QModelIndex& index);
void clearFocusAnimations(QWidget* scope);
bool ensureItemVisible(QAbstractItemView* view, const QModelIndex& index,
                       SmoothScrollController* controller = nullptr);
bool ensureGlobalRectVisible(QScrollArea* scrollArea,
                             const QRect& targetGlobalRect,
                             SmoothScrollController* controller = nullptr);
void ensureFocusedWidgetVisible(QWidget* widget);
} // namespace InputNavigation

#endif
