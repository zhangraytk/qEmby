#ifndef HORIZONTALLISTVIEWGALLERY_H
#define HORIZONTALLISTVIEWGALLERY_H

#include <QWidget>
#include <QPoint>
#include <QList>
#include <QSize>
#include <models/media/mediaitem.h>
#include "../utils/inputnavigation.h"
#include "../utils/wheelinput.h"

class ShimmerWidget;
#include "../views/media/mediacarddelegate.h"

class QEmbyCore;
class QListView;
class QPushButton;
class QPropertyAnimation;
class QNativeGestureEvent;
class QWheelEvent;
class MediaListModel; 

class HorizontalListViewGallery : public QWidget
{
    Q_OBJECT
public:
    
    explicit HorizontalListViewGallery(QEmbyCore* core, QWidget* parent = nullptr);
    ~HorizontalListViewGallery() override;

    
    QListView* listView() const { return m_listView; }

    
    void setItems(const QList<MediaItem>& items);
    void updateItem(const MediaItem& item);
    void prependOrUpdateItem(const MediaItem& item, int maxItems = 0);
    
    
    void removeItem(const QString& itemId);
    int itemCount() const;
    void clearImageCache();

    
    void setCardStyle(MediaCardDelegate::CardStyle style);

    
    void setTileSize(const QSize &size);
    void setTextPixelSizes(int titlePx, int subTitlePx);
    void setContentPadding(int padding);
    void setHoverControls(MediaCardDelegate::HoverControls controls);
    void scrollToItemId(const QString &itemId);
    void scrollPreviousPage();
    void scrollNextPage();
    bool handleHorizontalWheel(QWheelEvent* event);
    bool handleHorizontalPan(QNativeGestureEvent* event);
    bool moveFocus(int delta);
    bool activateFocusedItem();
    bool hasFocusedItem() const;
    int focusedRow() const;
    QRect focusedItemGlobalRect(int padding = 6) const;
    bool focusClosestInDirection(const QRect& sourceGlobalRect,
                                 NavigationCommand command);
    void setFocusedRow(int row);
    void clearKeyboardFocus();

    
    void setHighlightedItemId(const QString &id);

    
    void setLoading(bool loading);

    
    QSize minimumSizeHint() const override {
        return QSize(1, QWidget::minimumSizeHint().height());
    }

Q_SIGNALS:
    
    void itemClicked(const MediaItem& item);
    void playRequested(const MediaItem& item);
    void favoriteRequested(const MediaItem& item);
    void moreMenuRequested(const MediaItem& item, const QPoint& globalPos);

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    int clampedRow(int row) const;
    void setKeyboardFocusRow(int row, bool center);
    void scrollPage(int directionMultiplier);
    void updateButtonsVisibility();
    void updateButtonPositions();
    void updateVisibleImagePriority();

    QEmbyCore* m_core;
    QListView* m_listView;
    QPushButton* m_btnLeft;
    QPushButton* m_btnRight;

    
    MediaListModel* m_listModel;
    MediaCardDelegate* m_listDelegate;

    
    QPropertyAnimation* m_hScrollAnim;
    int m_hScrollTarget;
    int m_keyboardFocusRow = -1;
    bool m_keyboardFocusActive = false;
    WheelInput::AxisLock m_wheelAxisLock;

    
    MediaCardDelegate::CardStyle m_cardStyle;

    
    ShimmerWidget* m_shimmer = nullptr;
};

#endif 
