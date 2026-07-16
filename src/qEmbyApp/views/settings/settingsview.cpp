#include "settingsview.h"
#include "../../components/slidingstackedwidget.h"
#include "../../managers/thememanager.h" 
#include "../../utils/inputnavigation.h"
#include "../../utils/smoothscrollcontroller.h"
#include "pageabout.h"
#include "pageappearance.h"
#include "pagecontrols.h"
#include "pagegeneral.h"
#include "pageplayer.h"
#include "pagelibrary.h"
#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QNativeGestureEvent>


SettingsView::SettingsView(QEmbyCore *core, QWidget *parent)
    : BaseView(core, parent) {
  
  setAttribute(Qt::WA_StyledBackground, true);
  setObjectName("SettingsRootView");

  setupUi();
  setupConnections();

  
  
  m_navMenu->setCurrentRow(0);
}

void SettingsView::setupUi() {
  auto *mainLayout = new QHBoxLayout(this);
  
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  
  m_leftPanel = new QWidget(this);
  m_leftPanel->setFixedWidth(260);
  m_leftPanel->setObjectName("SettingsLeftPanel");
  auto *leftLayout = new QVBoxLayout(m_leftPanel);
  
  leftLayout->setContentsMargins(32, 32, 32, 32);
  leftLayout->setSpacing(24);

  
  m_titleLabel = new QLabel(tr("Settings"), m_leftPanel);
  m_titleLabel->setObjectName("SettingsMainTitle");

  
  m_navMenu = new QListWidget(m_leftPanel);
  m_navMenu->setObjectName("SettingsNavMenu");
  m_navMenu->setFocusPolicy(Qt::StrongFocus);
  
  m_navMenu->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_navMenu->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  
  auto *itemGeneral = new QListWidgetItem(
      ThemeManager::getAdaptiveIcon(":/svg/dark/general.svg"), tr(" General"));
  itemGeneral->setData(Qt::UserRole, ":/svg/dark/general.svg");

  auto *itemAppearance = new QListWidgetItem(
      ThemeManager::getAdaptiveIcon(":/svg/dark/appearance.svg"),
      tr(" Appearance"));
  itemAppearance->setData(Qt::UserRole, ":/svg/dark/appearance.svg");

  auto *itemControls = new QListWidgetItem(
      ThemeManager::getAdaptiveIcon(":/svg/dark/settings.svg"),
      tr(" Controls"));
  itemControls->setData(Qt::UserRole, ":/svg/dark/settings.svg");

  auto *itemPlayer = new QListWidgetItem(
      ThemeManager::getAdaptiveIcon(":/svg/dark/player.svg"), tr(" Player"));
  itemPlayer->setData(Qt::UserRole, ":/svg/dark/player.svg");

  auto *itemLibrary = new QListWidgetItem(
      ThemeManager::getAdaptiveIcon(":/svg/dark/library.svg"), tr(" Library"));
  itemLibrary->setData(Qt::UserRole, ":/svg/dark/library.svg");

  auto *itemAbout = new QListWidgetItem(
      ThemeManager::getAdaptiveIcon(":/svg/dark/about.svg"), tr(" About"));
  itemAbout->setData(Qt::UserRole, ":/svg/dark/about.svg");

  
  itemGeneral->setSizeHint(QSize(220, 44));
  itemAppearance->setSizeHint(QSize(220, 44));
  itemControls->setSizeHint(QSize(220, 44));
  itemPlayer->setSizeHint(QSize(220, 44));
  itemLibrary->setSizeHint(QSize(220, 44));
  itemAbout->setSizeHint(QSize(220, 44));

  m_navMenu->addItem(itemGeneral);
  m_navMenu->addItem(itemAppearance);
  m_navMenu->addItem(itemControls);
  m_navMenu->addItem(itemLibrary);
  m_navMenu->addItem(itemPlayer);
  m_navMenu->addItem(itemAbout);

  leftLayout->addWidget(m_titleLabel);
  leftLayout->addWidget(m_navMenu);

  
  m_stack = new SlidingStackedWidget(this);
  m_stack->setObjectName("SettingsStack");

  
  
  
  
  const int kPageCount = 6;
  m_scrollAreas.reserve(kPageCount);
  m_scrollControllers.reserve(kPageCount);
  m_pages.reserve(kPageCount);
  for (int i = 0; i < kPageCount; ++i) {
    auto *placeholder = new QWidget(m_stack);
    placeholder->setAttribute(Qt::WA_StyledBackground, true);
    placeholder->setObjectName("SettingsPagePlaceholder");
    m_stack->addWidget(placeholder);

    m_scrollAreas.append(nullptr);
    m_scrollControllers.append(nullptr);
    m_pages.append(QPointer<QWidget>());
  }

  mainLayout->addWidget(m_leftPanel);
  mainLayout->addWidget(m_stack, 1);

  
  qApp->postEvent(this, new QEvent(QEvent::StyleChange));
}

QScrollArea *SettingsView::wrapInScrollArea(QWidget *page, int row) {
  
  page->setAttribute(Qt::WA_StyledBackground, true);

  auto *scroll = new QScrollArea(m_stack);
  scroll->setObjectName("SettingsScrollArea");
  scroll->setWidget(page);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);

  
  scroll->viewport()->setAutoFillBackground(false);

  
  scroll->viewport()->installEventFilter(this);

  
  if (row >= 0 && row < m_scrollAreas.size()) {
    m_scrollAreas[row] = scroll;

    auto *controller =
        new SmoothScrollController(scroll->verticalScrollBar(), this);
    controller->setDuration(160);
    m_scrollControllers[row] = controller;
  }

  return scroll;
}

void SettingsView::ensurePageAt(int row) {
  if (row < 0 || row >= m_pages.size()) {
    return;
  }
  if (m_pages[row]) {
    
    return;
  }

  qDebug() << "[SettingsView] Lazy instantiate page row=" << row;

  QWidget *page = nullptr;
  switch (row) {
  case 0:
    page = new PageGeneral(m_core, m_stack);
    break;
  case 1:
    page = new PageAppearance(m_core, m_stack);
    break;
  case 2:
    page = new PageControls(m_core, m_stack);
    break;
  case 3:
    page = new PageLibrary(m_core, m_stack);
    break;
  case 4:
    page = new PagePlayer(m_core, m_stack);
    break;
  case 5:
    page = new PageAbout(m_core, m_stack);
    break;
  default:
    return;
  }

  QScrollArea *scroll = wrapInScrollArea(page, row);

  
  
  QWidget *placeholder = m_stack->widget(row);
  const bool wasBlocked = m_stack->blockSignals(true);
  m_stack->insertWidget(row, scroll); 
  m_stack->removeWidget(placeholder);
  m_stack->blockSignals(wasBlocked);
  placeholder->deleteLater();

  m_pages[row] = scroll;
}

void SettingsView::setupConnections() {
  
  connect(m_navMenu, &QListWidget::currentRowChanged, this, [this](int row) {
    if (row >= 0) {
      ensurePageAt(row);
      m_stack->setCurrentIndex(row);
    }
  });

  
  connect(ThemeManager::instance(), &ThemeManager::themeChanged, this,
          &SettingsView::onThemeChanged);
}


void SettingsView::onThemeChanged() {
  for (int i = 0; i < m_navMenu->count(); ++i) {
    auto *item = m_navMenu->item(i);
    
    QString svgPath = item->data(Qt::UserRole).toString();
    if (!svgPath.isEmpty()) {
      
      item->setIcon(ThemeManager::getAdaptiveIcon(svgPath));
    }
  }
}

bool SettingsView::handleRemoteNavigation(NavigationCommand command)
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
      ensurePageAt(row);
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

void SettingsView::setRemoteFocusActive(bool active)
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

bool SettingsView::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::Wheel) {
    
    for (int i = 0; i < m_scrollAreas.size(); ++i) {
      
      if (!m_scrollAreas[i]) {
        continue;
      }
      if (obj == m_scrollAreas[i]->viewport()) {
        auto *we = static_cast<QWheelEvent *>(event);
        auto *controller = i < m_scrollControllers.size()
                               ? m_scrollControllers[i]
                               : nullptr;
        if (controller) {
          const bool handled =
              controller->scrollByWheelEvent(we, Qt::Vertical);
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
        if (m_scrollAreas[i] && obj == m_scrollAreas[i]->viewport() &&
            i < m_scrollControllers.size() && m_scrollControllers[i]) {
          return m_scrollControllers[i]->scrollByNativeGesture(
              gesture, Qt::Vertical);
        }
      }
    }
  }

  return QWidget::eventFilter(obj, event);
}
