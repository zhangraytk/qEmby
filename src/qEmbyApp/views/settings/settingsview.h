#ifndef SETTINGSVIEW_H
#define SETTINGSVIEW_H

#include "../baseview.h"
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QPointer>
#include <QScrollArea>
#include <QVBoxLayout>

class SlidingStackedWidget;
class SmoothScrollController;

class SettingsView : public BaseView {
    Q_OBJECT
public:
    explicit SettingsView(QEmbyCore* core, QWidget* parent = nullptr);
    ~SettingsView() override = default;
    bool handleRemoteNavigation(NavigationCommand command) override;
    void setRemoteFocusActive(bool active) override;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUi();
    void setupConnections();

    
    
    void ensurePageAt(int row);

    
    
    QScrollArea* wrapInScrollArea(QWidget* page, int row);

private slots:
    
    void onThemeChanged();

private:
    QWidget* m_leftPanel;
    QLabel* m_titleLabel;
    QListWidget* m_navMenu;
    SlidingStackedWidget* m_stack;

    
    
    QList<QScrollArea*>        m_scrollAreas;
    QList<SmoothScrollController*> m_scrollControllers;

    
    QList<QPointer<QWidget>>   m_pages;
};

#endif 
