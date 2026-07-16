#include "horizontallistviewgallery.h"
#include "shimmerwidget.h"
#include "../utils/inputnavigation.h"
#include "../utils/textwraputils.h"
#include "../utils/wheelinput.h"
#include "../views/media/medialistmodel.h"
#include <QListView>
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QPropertyAnimation>
#include <QEvent>
#include <QWheelEvent>
#include <QNativeGestureEvent>
#include <QCursor>
#include <QScroller>           
#include <QScrollerProperties> 
#include <QStyleOptionViewItem>
#include <QTimer>
#include <QDebug>
#include <QSet>
#include <algorithm>

HorizontalListViewGallery::HorizontalListViewGallery(QEmbyCore* core, QWidget* parent)
    : QWidget(parent), m_core(core), m_hScrollAnim(nullptr), m_hScrollTarget(0), m_cardStyle(MediaCardDelegate::Poster)
{
    setObjectName("horizontal-listview-gallery");
    setAttribute(Qt::WA_StyledBackground, true);
    setMouseTracking(true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_listView = new QListView(this);
    m_listView->setViewMode(QListView::IconMode);
    m_listView->setFlow(QListView::LeftToRight);
    m_listView->setWrapping(false);
    m_listView->setSpacing(0);
    m_listView->setMovement(QListView::Static);
    m_listView->setResizeMode(QListView::Adjust);
    m_listView->setUniformItemSizes(true);

    m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listView->setMouseTracking(true);
    m_listView->viewport()->setAttribute(Qt::WA_Hover);
    m_listView->setStyleSheet("QListView { background: transparent; outline: none; border: none; }");
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setFocusPolicy(Qt::NoFocus);

    
    
    
    
    m_listView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    
    QScroller::grabGesture(m_listView->viewport(), QScroller::LeftMouseButtonGesture);
    QScroller* scroller = QScroller::scroller(m_listView->viewport());
    QScrollerProperties props = scroller->scrollerProperties();
    
    props.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    props.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    
    props.setScrollMetric(QScrollerProperties::DragStartDistance, 0.001);
    scroller->setScrollerProperties(props);

    
    m_hScrollAnim = new QPropertyAnimation(m_listView->horizontalScrollBar(), "value", this);
    m_hScrollAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_hScrollAnim->setDuration(220);

    mainLayout->addWidget(m_listView);

    
    m_shimmer = new ShimmerWidget(this);
    m_shimmer->hide();

    
    
    
    m_listModel = new MediaListModel(400, m_core, this);
    m_listDelegate = new MediaCardDelegate(MediaCardDelegate::Poster, this);

    m_listView->setModel(m_listModel);
    m_listView->setItemDelegate(m_listDelegate);

    
    connect(m_listDelegate, &MediaCardDelegate::playRequested, this, &HorizontalListViewGallery::playRequested);
    connect(m_listDelegate, &MediaCardDelegate::favoriteRequested, this, &HorizontalListViewGallery::favoriteRequested);
    connect(m_listDelegate, &MediaCardDelegate::moreMenuRequested, this, &HorizontalListViewGallery::moreMenuRequested);

    
    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        clearKeyboardFocus();
        if (m_listModel) {
            Q_EMIT itemClicked(m_listModel->getItem(index));
        }
    });

    
    
    
    m_btnLeft = new QPushButton("❮", this);
    m_btnRight = new QPushButton("❯", this);

    QString btnStyle = "QPushButton { background-color: rgba(0,0,0,120); color: white; font-size: 20px; border: none; border-radius: 8px; }"
                       "QPushButton:hover { background-color: rgba(60,60,60,200); }";
    m_btnLeft->setStyleSheet(btnStyle);
    m_btnRight->setStyleSheet(btnStyle);

    m_btnLeft->setFixedSize(40, 60);
    m_btnRight->setFixedSize(40, 60);
    m_btnLeft->setCursor(Qt::PointingHandCursor);
    m_btnRight->setCursor(Qt::PointingHandCursor);
    m_btnLeft->setFocusPolicy(Qt::NoFocus);
    m_btnRight->setFocusPolicy(Qt::NoFocus);

    m_btnLeft->hide();
    m_btnRight->hide();

    connect(m_btnLeft, &QPushButton::clicked,
            this, &HorizontalListViewGallery::scrollPreviousPage);
    connect(m_btnRight, &QPushButton::clicked,
            this, &HorizontalListViewGallery::scrollNextPage);

    connect(m_listView->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            [this]() {
                updateButtonsVisibility();
                updateVisibleImagePriority();
            });
    connect(m_listView->horizontalScrollBar(), &QScrollBar::rangeChanged, this, &HorizontalListViewGallery::updateButtonsVisibility);

    m_listView->viewport()->installEventFilter(this);
    this->installEventFilter(this);
}

HorizontalListViewGallery::~HorizontalListViewGallery()
{
    if (m_hScrollAnim) {
        m_hScrollAnim->stop();
    }
    if (m_listView && m_listView->viewport()) {
        m_listView->viewport()->removeEventFilter(this);
        QScroller::ungrabGesture(m_listView->viewport());
    }
    removeEventFilter(this);
}


void HorizontalListViewGallery::setItems(const QList<MediaItem>& items)
{
    if (m_listModel) {
        m_listModel->setItems(items);
        if (m_keyboardFocusRow >= m_listModel->rowCount()) {
            clearKeyboardFocus();
        } else if (m_keyboardFocusActive) {
            setKeyboardFocusRow(m_keyboardFocusRow, false);
        }
        QTimer::singleShot(0, this,
                           [this]() { updateVisibleImagePriority(); });
    }
    
    if (m_shimmer && m_shimmer->isVisible() && !items.isEmpty()) {
        m_shimmer->stopAnimation();
        m_shimmer->hide();
    }
}


void HorizontalListViewGallery::updateItem(const MediaItem& item)
{
    if (m_listModel) {
        m_listModel->updateItem(item);
    }
}

void HorizontalListViewGallery::prependOrUpdateItem(const MediaItem& item,
                                                    int maxItems)
{
    if (m_listModel) {
        m_listModel->prependOrUpdateItem(item, maxItems);
        QTimer::singleShot(0, this, [this]() {
            updateVisibleImagePriority();
            updateButtonsVisibility();
        });
    }
}


void HorizontalListViewGallery::removeItem(const QString& itemId)
{
    if (m_listModel) {
        const int previousCount = m_listModel->rowCount();
        m_listModel->removeItem(itemId);
        if (m_keyboardFocusRow >= m_listModel->rowCount()) {
            clearKeyboardFocus();
        }
        if (m_listView && m_listModel->rowCount() < previousCount) {
            m_listView->doItemsLayout();
            m_listView->updateGeometry();
            m_listView->update();
            m_listView->viewport()->update();
            QTimer::singleShot(0, this, [this]() {
                if (!m_listView) {
                    return;
                }

                m_listView->doItemsLayout();
                m_listView->viewport()->update();
                updateButtonsVisibility();
            });
        }
    }
}

int HorizontalListViewGallery::itemCount() const
{
    return m_listModel ? m_listModel->rowCount() : 0;
}

void HorizontalListViewGallery::clearImageCache()
{
    if (m_listModel) {
        m_listModel->clearImageCache();
    }
}

void HorizontalListViewGallery::setCardStyle(MediaCardDelegate::CardStyle style)
{
    bool styleChanged = (m_cardStyle != style);
    m_cardStyle = style; 
    if (m_listDelegate) {
        m_listDelegate->setStyle(style);

        
        if (style == MediaCardDelegate::LibraryTile) {
            int imgHeight = 160;
            int imgWidth = qRound(imgHeight * 16.0 / 9.0); 
            int cardWidth = imgWidth + 16;  
            const int hoverExpandH = qRound(imgHeight * 0.035);
            const int cardHeight =
                8 + imgHeight + hoverExpandH + 4 + 20 + 1 + 18 + 8;
            m_listDelegate->setTileSize(QSize(cardWidth, cardHeight));
        } else if (style == MediaCardDelegate::Poster) {
            m_listDelegate->setTileSize(QSize(160, 270));
        }

        
        m_listView->doItemsLayout();
        m_listView->viewport()->update();
    }
    
    if (m_listModel) {
        m_listModel->setPreferThumb(style == MediaCardDelegate::LibraryTile || style == MediaCardDelegate::EpisodeList);
        
        if (styleChanged) {
            m_listModel->clearImageCache();
        }
    }
    
    updateButtonPositions();
}

void HorizontalListViewGallery::setTileSize(const QSize &size)
{
    if (m_listDelegate) {
        m_listDelegate->setTileSize(size);
        m_listView->doItemsLayout();
        m_listView->viewport()->update();
    }
    updateButtonPositions();
}

void HorizontalListViewGallery::setTextPixelSizes(int titlePx, int subTitlePx)
{
    if (m_listDelegate) {
        m_listDelegate->setTextPixelSizes(titlePx, subTitlePx);
        m_listView->doItemsLayout();
        m_listView->viewport()->update();
    }
}

void HorizontalListViewGallery::setContentPadding(int padding)
{
    if (m_listDelegate) {
        m_listDelegate->setContentPadding(padding);
        m_listView->doItemsLayout();
        m_listView->viewport()->update();
    }
    updateButtonPositions();
}

void HorizontalListViewGallery::setHoverControls(
    MediaCardDelegate::HoverControls controls)
{
    if (m_listDelegate) {
        m_listDelegate->setHoverControls(controls);
        m_listView->viewport()->update();
    }
}

void HorizontalListViewGallery::scrollToItemId(const QString &itemId)
{
    const QString targetItemId = itemId.trimmed();
    if (targetItemId.isEmpty() || !m_listView || !m_listModel) {
        return;
    }

    QTimer::singleShot(0, this, [this, targetItemId]() {
        if (!m_listView || !m_listModel) {
            return;
        }

        m_listView->doItemsLayout();
        const int rowCount = m_listModel->rowCount();
        for (int row = 0; row < rowCount; ++row) {
            const QModelIndex index = m_listModel->index(row, 0);
            const MediaItem item =
                index.data(MediaListModel::ItemDataRole).value<MediaItem>();
            if (item.id != targetItemId) {
                continue;
            }

            qDebug() << "[HorizontalListViewGallery] Scroll to item"
                     << "| itemId=" << targetItemId
                     << "| row=" << row;
            m_listView->scrollTo(index, QAbstractItemView::PositionAtCenter);
            return;
        }

        qDebug() << "[HorizontalListViewGallery] Scroll target not found"
                 << "| itemId=" << targetItemId
                 << "| rowCount=" << rowCount;
    });
}

void HorizontalListViewGallery::setHighlightedItemId(const QString &id)
{
    if (m_listDelegate) {
        m_listDelegate->setHighlightedItemId(id);
        m_listView->viewport()->update();
    }
}

void HorizontalListViewGallery::scrollPreviousPage()
{
    scrollPage(-1);
}

void HorizontalListViewGallery::scrollNextPage()
{
    scrollPage(1);
}

bool HorizontalListViewGallery::handleHorizontalWheel(QWheelEvent* event)
{
    if (!event ||
        m_wheelAxisLock.axisFor(event) != WheelInput::Axis::Horizontal) {
        if (event) {
            event->ignore();
        }
        return false;
    }

    clearKeyboardFocus();

    if (!m_listView || !m_hScrollAnim || !m_hScrollAnim->targetObject()) {
        event->ignore();
        return false;
    }

    QScrollBar* hBar = m_listView->horizontalScrollBar();
    if (!hBar) {
        event->ignore();
        return false;
    }

    const int step = WheelInput::delta(event, Qt::Horizontal);
    if (step == 0) {
        event->ignore();
        return false;
    }

    if (WheelInput::isPrecise(event)) {
        m_hScrollAnim->stop();
        const int currentValue = hBar->value();
        const int nextValue = qBound(hBar->minimum(), currentValue - step,
                                     hBar->maximum());
        m_hScrollTarget = nextValue;
        if (nextValue == currentValue) {
            event->ignore();
            return false;
        }
        hBar->setValue(nextValue);
        event->accept();
        return true;
    }

    int currentVal = hBar->value();
    if (m_hScrollAnim->state() == QAbstractAnimation::Running) {
        currentVal = m_hScrollTarget;
    }

    const int newTarget =
        qBound(hBar->minimum(), currentVal - step, hBar->maximum());
    if (newTarget == currentVal) {
        event->ignore();
        return false;
    }

    m_hScrollTarget = newTarget;
    m_hScrollAnim->stop();
    m_hScrollAnim->setDuration(220);
    m_hScrollAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_hScrollAnim->setStartValue(hBar->value());
    m_hScrollAnim->setEndValue(m_hScrollTarget);
    m_hScrollAnim->start();

    event->accept();
    return true;
}

bool HorizontalListViewGallery::handleHorizontalPan(QNativeGestureEvent* event)
{
    if (!event || event->gestureType() != Qt::PanNativeGesture ||
        qAbs(event->delta().x()) <= qAbs(event->delta().y()) || !m_listView) {
        return false;
    }

    QScrollBar* bar = m_listView->horizontalScrollBar();
    if (!bar) {
        return false;
    }

    m_hScrollAnim->stop();
    const int currentValue = bar->value();
    const int nextValue = qBound(bar->minimum(),
                                 currentValue - qRound(event->delta().x()),
                                 bar->maximum());
    m_hScrollTarget = nextValue;
    if (nextValue == currentValue) {
        return false;
    }
    bar->setValue(nextValue);
    event->accept();
    return true;
}

bool HorizontalListViewGallery::moveFocus(int delta)
{
    if (!m_listView || !m_listModel || m_listModel->rowCount() <= 0) {
        return false;
    }

    int row = m_keyboardFocusActive ? m_keyboardFocusRow : -1;
    if (row < 0) {
        row = 0;
    } else {
        row += delta;
        if (row < 0 || row >= m_listModel->rowCount()) {
            return false;
        }
    }

    setKeyboardFocusRow(row, true);
    return true;
}

bool HorizontalListViewGallery::activateFocusedItem()
{
    if (!m_keyboardFocusActive || !m_listModel) {
        return false;
    }

    const int row = clampedRow(m_keyboardFocusRow);
    if (row < 0) {
        return false;
    }

    Q_EMIT itemClicked(m_listModel->getItem(m_listModel->index(row, 0)));
    return true;
}

bool HorizontalListViewGallery::hasFocusedItem() const
{
    return m_keyboardFocusActive && m_keyboardFocusRow >= 0;
}

int HorizontalListViewGallery::focusedRow() const
{
    return hasFocusedItem() ? m_keyboardFocusRow : -1;
}

QRect HorizontalListViewGallery::focusedItemGlobalRect(int padding) const
{
    if (!hasFocusedItem() || !m_listView || !m_listModel) {
        return {};
    }
    return InputNavigation::itemGlobalRect(
        m_listView, m_listModel->index(m_keyboardFocusRow, 0), padding);
}

bool HorizontalListViewGallery::focusClosestInDirection(
    const QRect& sourceGlobalRect, NavigationCommand command)
{
    if (!m_listView || !m_listModel || !sourceGlobalRect.isValid()) {
        return false;
    }
    m_listView->doItemsLayout();
    const QModelIndex index = InputNavigation::bestItemInDirection(
        m_listView, sourceGlobalRect, command);
    if (!index.isValid()) {
        return false;
    }
    // The candidate was chosen from its current on-screen geometry. Preserve
    // that horizontal alignment when crossing rows instead of re-pivoting it.
    setKeyboardFocusRow(index.row(), false);
    return true;
}

void HorizontalListViewGallery::setFocusedRow(int row)
{
    setKeyboardFocusRow(row, true);
}

void HorizontalListViewGallery::clearKeyboardFocus()
{
    m_keyboardFocusActive = false;
    m_keyboardFocusRow = -1;
    if (m_listView) {
        m_listView->clearSelection();
        m_listView->setCurrentIndex(QModelIndex());
        m_listView->viewport()->update();
    }
}

int HorizontalListViewGallery::clampedRow(int row) const
{
    if (!m_listModel || m_listModel->rowCount() <= 0) {
        return -1;
    }
    return qBound(0, row, m_listModel->rowCount() - 1);
}

void HorizontalListViewGallery::setKeyboardFocusRow(int row, bool center)
{
    const int targetRow = clampedRow(row);
    if (targetRow < 0 || !m_listView || !m_listModel) {
        clearKeyboardFocus();
        return;
    }

    m_keyboardFocusActive = true;
    m_keyboardFocusRow = targetRow;

    const QModelIndex index = m_listModel->index(targetRow, 0);
    m_listView->selectionModel()->select(
        index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    m_listView->setCurrentIndex(index);
    if (m_listDelegate) {
        m_listDelegate->animateRemoteFocus(index, m_listView->viewport());
    }

    if (center && m_hScrollAnim) {
        m_listView->doItemsLayout();
        const QRect itemRect = m_listView->visualRect(index);
        QScrollBar* bar = m_listView->horizontalScrollBar();
        if (bar && itemRect.isValid()) {
            // TV lists keep the focused card near a stable leading pivot.
            // The first card naturally clamps to the left edge.
            const int pivot = qRound(m_listView->viewport()->width() * 0.30);
            const int target = qBound(
                bar->minimum(), bar->value() + itemRect.left() - pivot,
                bar->maximum());
            if (target != bar->value()) {
                m_hScrollTarget = target;
                m_hScrollAnim->stop();
                m_hScrollAnim->setDuration(180);
                m_hScrollAnim->setEasingCurve(QEasingCurve::OutCubic);
                m_hScrollAnim->setStartValue(bar->value());
                m_hScrollAnim->setEndValue(target);
                m_hScrollAnim->start();
            }
        }
    }
    m_listView->viewport()->update();
}

void HorizontalListViewGallery::scrollPage(int directionMultiplier)
{
    if (!m_listView || !m_hScrollAnim || !m_hScrollAnim->targetObject()) {
        return;
    }

    QScrollBar* bar = m_listView->horizontalScrollBar();
    if (!bar || bar->maximum() <= 0) {
        return;
    }

    int currentValue = bar->value();
    if (m_hScrollAnim->state() == QAbstractAnimation::Running) {
        currentValue = m_hScrollTarget;
    }

    const int step = qMax(1, width() / 2);
    const int targetValue =
        qBound(bar->minimum(), currentValue + directionMultiplier * step,
               bar->maximum());
    if (targetValue == currentValue) {
        return;
    }

    m_hScrollTarget = targetValue;
    m_hScrollAnim->stop();
    m_hScrollAnim->setDuration(400);
    m_hScrollAnim->setStartValue(bar->value());
    m_hScrollAnim->setEndValue(m_hScrollTarget);
    m_hScrollAnim->start();
}

void HorizontalListViewGallery::setLoading(bool loading)
{
    if (!m_shimmer) {
        return;
    }

    if (loading) {
        
        QStyleOptionViewItem opt;
        const QSize cardSize = m_listDelegate->sizeHint(opt, QModelIndex());
        m_shimmer->setCardSize(cardSize);
        m_shimmer->setShowSubtitle(
            m_cardStyle == MediaCardDelegate::Poster ||
            m_cardStyle == MediaCardDelegate::Cast);
        m_shimmer->setGeometry(m_listView->geometry());
        m_shimmer->raise();
        m_shimmer->show();
        m_shimmer->startAnimation();
    } else {
        m_shimmer->stopAnimation();
        m_shimmer->hide();
    }
}

void HorizontalListViewGallery::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateButtonPositions();
    updateButtonsVisibility();
    updateVisibleImagePriority();

    
    if (m_shimmer && m_shimmer->isVisible()) {
        m_shimmer->setGeometry(m_listView->geometry());
    }
}

void HorizontalListViewGallery::updateButtonPositions()
{
    int currentWidth = this->width();
    
    
    QStyleOptionViewItem dummyOption;
    QModelIndex dummyIndex;
    QSize itemSize = m_listDelegate->sizeHint(dummyOption, dummyIndex);
    
    int padding = m_listDelegate ? m_listDelegate->contentPadding() : 8;
    int imgWidth = itemSize.width() - padding * 2;
    int imgHeight = 0;
    
    
    if (m_cardStyle == MediaCardDelegate::LibraryTile) {
        imgHeight = qRound(imgWidth * 9.0 / 16.0);
    } else {
        
        imgHeight = qRound(imgWidth * 1.5);
    }
    
    
    int imageCenterY = padding + (imgHeight / 2);
    int btnY = imageCenterY - (m_btnLeft->height() / 2);

    m_btnLeft->move(10, btnY);
    m_btnRight->move(currentWidth - m_btnRight->width() - 10, btnY);

    m_btnLeft->raise();
    m_btnRight->raise();
}

bool HorizontalListViewGallery::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::Enter || event->type() == QEvent::MouseMove) {
        clearKeyboardFocus();
        updateButtonsVisibility();
    } else if (event->type() == QEvent::Leave) {
        QPoint globalPos = QCursor::pos();
        if (!this->rect().contains(this->mapFromGlobal(globalPos))) {
            m_btnLeft->hide();
            m_btnRight->hide();
        }
    } else if (obj == m_listView->viewport() && event->type() == QEvent::ToolTip) {
        return TextWrapUtils::showWrappedMediaItemToolTip(m_listView, event);
    } else if (event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        return handleHorizontalWheel(wheelEvent);
    } else if (event->type() == QEvent::NativeGesture) {
        auto* gestureEvent = static_cast<QNativeGestureEvent*>(event);
        return handleHorizontalPan(gestureEvent);
    }
    return QWidget::eventFilter(obj, event);
}

void HorizontalListViewGallery::updateButtonsVisibility()
{
    QPoint globalPos = QCursor::pos();
    QPoint localPos = this->mapFromGlobal(globalPos);

    if (!this->rect().contains(localPos)) {
        m_btnLeft->hide();
        m_btnRight->hide();
        return;
    }

    int currentWidth = this->width();
    QScrollBar* bar = m_listView->horizontalScrollBar();

    bool isLeftHalf = localPos.x() < (currentWidth / 2);

    m_btnLeft->setVisible(isLeftHalf && bar->value() > 0);
    m_btnRight->setVisible(!isLeftHalf && bar->value() < bar->maximum());
}

void HorizontalListViewGallery::updateVisibleImagePriority()
{
    if (!m_listView || !m_listModel) {
        return;
    }

    QWidget* viewport = m_listView->viewport();
    if (!viewport) {
        return;
    }

    QStyleOptionViewItem option;
    const QSize cellSize = m_listDelegate->sizeHint(option, QModelIndex());
    const int stepX = qMax(1, cellSize.width() / 2);
    const int stepY = qMax(1, cellSize.height() / 2);

    QSet<int> rowSet;
    const QRect rect = viewport->rect();
    for (int y = rect.top(); y <= rect.bottom(); y += stepY) {
        for (int x = rect.left(); x <= rect.right(); x += stepX) {
            const QModelIndex idx = m_listView->indexAt(QPoint(x, y));
            if (idx.isValid()) {
                rowSet.insert(idx.row());
            }
        }
    }

    QList<int> rows = rowSet.values();
    std::sort(rows.begin(), rows.end());
    m_listModel->setPriorityRows(rows);
}
