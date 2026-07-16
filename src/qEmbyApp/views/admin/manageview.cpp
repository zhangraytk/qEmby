#include "manageview.h"
#include "managepagebase.h"
#include "pagedashboard.h"
#include "pagecollections.h"
#include "pagelibraries.h"
#include "pagelogs.h"
#include "pagetasks.h"
#include "pagetranscoding.h"
#include "pageusers.h"
#include "../../components/slidingstackedwidget.h"
#include "../../managers/thememanager.h"
#include "../../utils/inputnavigation.h"
#include "../../utils/smoothscrollcontroller.h"
#include <qembycore.h>
#include <services/manager/servermanager.h>
#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QShowEvent>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QNativeGestureEvent>

ManageView::ManageView(QEmbyCore *core, QWidget *parent)
    : BaseView(core, parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("SettingsRootView"); 

    setupUi();
    setupConnections();
}





void ManageView::showEvent(QShowEvent* event)
{
    BaseView::showEvent(event);
    m_core->serverManager()->connectWebSocket();
    qDebug() << "[ManageView] Entered management view — WebSocket connecting";
}

void ManageView::hideEvent(QHideEvent* event)
{
    m_core->serverManager()->disconnectWebSocket();
    qDebug() << "[ManageView] Left management view — WebSocket disconnected";
    BaseView::hideEvent(event);
}

void ManageView::setupUi() {
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    
    m_leftPanel = new QWidget(this);
    m_leftPanel->setFixedWidth(260);
    m_leftPanel->setObjectName("SettingsLeftPanel"); 
    auto *leftLayout = new QVBoxLayout(m_leftPanel);
    leftLayout->setContentsMargins(32, 32, 32, 32);
    leftLayout->setSpacing(24);

    m_titleLabel = new QLabel(tr("Management"), m_leftPanel);
    m_titleLabel->setObjectName("SettingsMainTitle"); 

    m_navMenu = new QListWidget(m_leftPanel);
    m_navMenu->setObjectName("SettingsNavMenu"); 
    m_navMenu->setFocusPolicy(Qt::StrongFocus);
    m_navMenu->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_navMenu->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    
    auto *itemDashboard = new QListWidgetItem(
        ThemeManager::getAdaptiveIcon(":/svg/dark/manage-dashboard.svg"), tr(" Dashboard"));
    itemDashboard->setData(Qt::UserRole, ":/svg/dark/manage-dashboard.svg");

    auto *itemLibraries = new QListWidgetItem(
        ThemeManager::getAdaptiveIcon(":/svg/dark/manage-libraries.svg"), tr(" Libraries"));
    itemLibraries->setData(Qt::UserRole, ":/svg/dark/manage-libraries.svg");

    auto *itemCollections = new QListWidgetItem(
        ThemeManager::getAdaptiveIcon(":/svg/dark/manage-collections.svg"), tr(" Lists"));
    itemCollections->setData(Qt::UserRole, ":/svg/dark/manage-collections.svg");

    auto *itemTranscoding = new QListWidgetItem(
        ThemeManager::getAdaptiveIcon(":/svg/dark/video-output.svg"), tr(" Transcoding"));
    itemTranscoding->setData(Qt::UserRole, ":/svg/dark/video-output.svg");

    auto *itemUsers = new QListWidgetItem(
        ThemeManager::getAdaptiveIcon(":/svg/dark/manage-users.svg"), tr(" Users"));
    itemUsers->setData(Qt::UserRole, ":/svg/dark/manage-users.svg");

    auto *itemTasks = new QListWidgetItem(
        ThemeManager::getAdaptiveIcon(":/svg/dark/manage-tasks.svg"), tr(" Tasks"));
    itemTasks->setData(Qt::UserRole, ":/svg/dark/manage-tasks.svg");

    auto *itemLogs = new QListWidgetItem(
        ThemeManager::getAdaptiveIcon(":/svg/dark/manage-logs.svg"), tr(" Logs"));
    itemLogs->setData(Qt::UserRole, ":/svg/dark/manage-logs.svg");

    
    QSize itemSize(220, 44);
    itemDashboard->setSizeHint(itemSize);
    itemLibraries->setSizeHint(itemSize);
    itemCollections->setSizeHint(itemSize);
    itemTranscoding->setSizeHint(itemSize);
    itemUsers->setSizeHint(itemSize);
    itemTasks->setSizeHint(itemSize);
    itemLogs->setSizeHint(itemSize);

    m_navMenu->addItem(itemDashboard);
    m_navMenu->addItem(itemLibraries);
    m_navMenu->addItem(itemCollections);
    m_navMenu->addItem(itemTranscoding);
    m_navMenu->addItem(itemUsers);
    m_navMenu->addItem(itemTasks);
    m_navMenu->addItem(itemLogs);

    leftLayout->addWidget(m_titleLabel);
    leftLayout->addWidget(m_navMenu);

    
    m_stack = new SlidingStackedWidget(this);
    m_stack->setObjectName("SettingsStack"); 

    
    auto wrapInScrollArea = [this](QWidget *page) -> QScrollArea * {
        page->setAttribute(Qt::WA_StyledBackground, true);

        auto *scroll = new QScrollArea();
        scroll->setObjectName("SettingsScrollArea");
        scroll->setWidget(page);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->viewport()->setAutoFillBackground(false);

        m_scrollAreas.append(scroll);
        scroll->viewport()->installEventFilter(this);

        return scroll;
    };

    
    m_stack->addWidget(wrapInScrollArea(new PageDashboard(m_core, m_stack)));

    
    auto *libPage = new PageLibraries(m_core, m_stack);
    libPage->setAttribute(Qt::WA_StyledBackground, true);
    m_stack->addWidget(libPage);
    
    m_scrollAreas.append(libPage->scrollArea());
    libPage->scrollArea()->viewport()->installEventFilter(this);

    
    auto *collPage = new PageCollections(m_core, m_stack);
    collPage->setAttribute(Qt::WA_StyledBackground, true);
    m_stack->addWidget(collPage);
    m_scrollAreas.append(collPage->scrollArea());
    collPage->scrollArea()->viewport()->installEventFilter(this);

    auto *transcodingPage = new PageTranscoding(m_core, m_stack);
    transcodingPage->setAttribute(Qt::WA_StyledBackground, true);
    m_stack->addWidget(transcodingPage);
    m_scrollAreas.append(transcodingPage->scrollArea());
    transcodingPage->scrollArea()->viewport()->installEventFilter(this);

    m_stack->addWidget(wrapInScrollArea(new PageUsers(m_core, m_stack)));

    auto *taskPage = new PageTasks(m_core, m_stack);
    taskPage->setAttribute(Qt::WA_StyledBackground, true);
    m_stack->addWidget(taskPage);
    m_scrollAreas.append(taskPage->scrollArea());
    taskPage->scrollArea()->viewport()->installEventFilter(this);

    auto *logPage = new PageLogs(m_core, m_stack);
    logPage->setAttribute(Qt::WA_StyledBackground, true);
    m_stack->addWidget(logPage);

    mainLayout->addWidget(m_leftPanel);
    mainLayout->addWidget(m_stack, 1);

    m_navMenu->setCurrentRow(0);

    
    for (auto *sa : m_scrollAreas) {
        auto *controller =
            new SmoothScrollController(sa->verticalScrollBar(), this);
        controller->setDuration(160);
        m_scrollControllers.append(controller);
    }

    qApp->postEvent(this, new QEvent(QEvent::StyleChange));
}

void ManageView::setupConnections() {
    connect(m_navMenu, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0) {
            m_stack->setCurrentIndex(row);
        }
    });

    connect(ThemeManager::instance(), &ThemeManager::themeChanged, this,
            &ManageView::onThemeChanged);
}

void ManageView::onThemeChanged() {
    for (int i = 0; i < m_navMenu->count(); ++i) {
        auto *item = m_navMenu->item(i);
        QString svgPath = item->data(Qt::UserRole).toString();
        if (!svgPath.isEmpty()) {
            item->setIcon(ThemeManager::getAdaptiveIcon(svgPath));
        }
    }
}

bool ManageView::handleRemoteNavigation(NavigationCommand command)
{
    QWidget *focused = QApplication::focusWidget();
    const bool focusInMenu = focused &&
        (focused == m_navMenu || m_navMenu->isAncestorOf(focused));
    if (!focusInMenu) {
        return BaseView::handleRemoteNavigation(command);
    }

    if (command == NavigationCommand::Up ||
        command == NavigationCommand::Down) {
        const int delta = command == NavigationCommand::Up ? -1 : 1;
        return InputNavigation::moveCurrentItem(m_navMenu, delta);
    }
    if (command == NavigationCommand::Activate) {
        const int row = m_navMenu->currentRow();
        if (row >= 0) {
            m_stack->setCurrentIndex(row);
            InputNavigation::animateItemFocus(
                m_navMenu, m_navMenu->currentIndex());
        }
        return true;
    }
    if (command == NavigationCommand::Right) {
        QWidget *page = m_stack->currentWidget();
        if (page) {
            InputNavigation::moveSpatialFocus(page, command);
        }
        return true;
    }
    if (command == NavigationCommand::Left) {
        return false;
    }
    return BaseView::handleRemoteNavigation(command);
}

void ManageView::setRemoteFocusActive(bool active)
{
    BaseView::setRemoteFocusActive(active);
    if (!active) {
        InputNavigation::clearFocusAnimations(this);
        return;
    }

    QWidget *focused = QApplication::focusWidget();
    if (!focused || (focused != this && !isAncestorOf(focused))) {
        InputNavigation::focusCurrentItem(m_navMenu);
    }
}

bool ManageView::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::Wheel) {
        for (int i = 0; i < m_scrollAreas.size(); ++i) {
            if (obj == m_scrollAreas[i]->viewport()) {
                auto *we   = static_cast<QWheelEvent *>(event);
                if (i < m_scrollControllers.size() &&
                    m_scrollControllers[i]) {
                    const bool handled = m_scrollControllers[i]
                        ->scrollByWheelEvent(we, Qt::Vertical);
                    if (handled) {
                        we->accept();
                    } else {
                        we->ignore();
                    }
                    return handled;
                }
                we->ignore();
                return false;
            }
        }
    }

    if (event->type() == QEvent::NativeGesture) {
        auto *gesture = static_cast<QNativeGestureEvent *>(event);
        if (gesture->gestureType() == Qt::PanNativeGesture) {
            for (int i = 0; i < m_scrollAreas.size(); ++i) {
                if (obj == m_scrollAreas[i]->viewport() &&
                    i < m_scrollControllers.size() &&
                    m_scrollControllers[i]) {
                    return m_scrollControllers[i]->scrollByNativeGesture(
                        gesture, Qt::Vertical);
                }
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}
