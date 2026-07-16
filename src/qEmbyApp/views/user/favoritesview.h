#ifndef FAVORITESVIEW_H
#define FAVORITESVIEW_H

#include "../baseview.h"
#include <QList>
#include <qcorotask.h>
#include "../../utils/wheelinput.h"
#include "../../utils/asyncrequestgate.h"

class QListView;
class QLabel;
class QScrollArea;
class HorizontalListViewGallery;
class SmoothScrollController;

class FavoritesView : public BaseView
{
    Q_OBJECT
public:
    explicit FavoritesView(QEmbyCore* core, QWidget *parent = nullptr);
    
    
    QCoro::Task<void> loadFavoritesData();
    bool handleRemoteNavigationKey(int key);
    void setRemoteFocusActive(bool active) override;

protected:
    
    void onMediaItemUpdated(const MediaItem& item) override;
    void onMediaItemRemoved(const QString& itemId) override;

    void showEvent(QShowEvent* event) override;
    
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUi();
    QString currentFavoritesContextKey() const;
    void clearFavoritesState();

    
    QWidget* createSectionHeader(const QString& title, const QString& itemType = QString());
    QList<HorizontalListViewGallery*> visibleFeedGalleries() const;
    HorizontalListViewGallery* activeFeedGallery() const;
    HorizontalListViewGallery* galleryForObject(QObject* obj) const;
    void clearFeedKeyboardFocuses();
    void ensureRemoteFocusVisible(HorizontalListViewGallery* gallery);

    QScrollArea* m_mainScrollArea;

    SmoothScrollController* m_vScrollController;
    WheelInput::AxisLock m_wheelAxisLock;

    
    QWidget* m_moviesHeader;
    HorizontalListViewGallery* m_moviesGallery;

    
    QWidget* m_seriesHeader;
    HorizontalListViewGallery* m_seriesGallery;

    
    QWidget* m_collectionsHeader;
    HorizontalListViewGallery* m_collectionsGallery;

    
    QWidget* m_playlistsHeader;
    HorizontalListViewGallery* m_playlistsGallery;

    
    QWidget* m_peopleHeader;
    HorizontalListViewGallery* m_peopleGallery;

    
    QWidget* m_foldersHeader;
    HorizontalListViewGallery* m_foldersGallery;

    AsyncRequestGate m_loadGate;
    QString m_favoritesContextKey;
};

#endif 
