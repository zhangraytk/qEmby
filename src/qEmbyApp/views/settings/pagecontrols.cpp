#include "pagecontrols.h"
#include "../../components/shortcutedit.h"
#include "../../components/settingscard.h"
#include "../../utils/shortcututils.h"
#include "config/config_keys.h"

PageControls::PageControls(QEmbyCore* core, QWidget* parent)
    : SettingsPageBase(core, tr("Controls"), parent)
{
    auto* backShortcutEdit = new ShortcutEdit(this);
    m_mainLayout->addWidget(new SettingsCard(
        ":/svg/dark/general.svg",
        tr("Back Shortcut"),
        tr("Capture a shortcut or type semicolon-separated alternatives"),
        backShortcutEdit,
        ConfigKeys::ShortcutNavigationBack,
        this,
        ShortcutUtils::defaultNavigationBackShortcuts()));

    auto* forwardShortcutEdit = new ShortcutEdit(this);
    m_mainLayout->addWidget(new SettingsCard(
        ":/svg/dark/general.svg",
        tr("Forward Shortcut"),
        tr("Shortcut for moving forward through browsing history"),
        forwardShortcutEdit,
        ConfigKeys::ShortcutNavigationForward,
        this,
        ShortcutUtils::defaultNavigationForwardShortcuts()));

    auto* homeShortcutEdit = new ShortcutEdit(this);
    m_mainLayout->addWidget(new SettingsCard(
        ":/svg/dark/home.svg",
        tr("Home Shortcut"),
        tr("Shortcut for returning to the home page"),
        homeShortcutEdit,
        ConfigKeys::ShortcutNavigationHome,
        this,
        ShortcutUtils::defaultNavigationHomeShortcut()));

    auto* favoritesShortcutEdit = new ShortcutEdit(this);
    m_mainLayout->addWidget(new SettingsCard(
        ":/svg/dark/heart.svg",
        tr("Favorites Shortcut"),
        tr("Shortcut for opening favorites"),
        favoritesShortcutEdit,
        ConfigKeys::ShortcutNavigationFavorites,
        this,
        ShortcutUtils::defaultNavigationFavoritesShortcut()));

    auto* feedPrevShortcutEdit = new ShortcutEdit(this);
    m_mainLayout->addWidget(new SettingsCard(
        ":/svg/dark/arrow-left.svg",
        tr("Feed Previous Page"),
        tr("Shortcut for scrolling the active home feed left"),
        feedPrevShortcutEdit,
        ConfigKeys::ShortcutFeedPreviousPage,
        this,
        ShortcutUtils::defaultFeedPreviousPageShortcuts()));

    auto* feedNextShortcutEdit = new ShortcutEdit(this);
    m_mainLayout->addWidget(new SettingsCard(
        ":/svg/dark/general.svg",
        tr("Feed Next Page"),
        tr("Shortcut for scrolling the active home feed right"),
        feedNextShortcutEdit,
        ConfigKeys::ShortcutFeedNextPage,
        this,
        ShortcutUtils::defaultFeedNextPageShortcuts()));

    m_mainLayout->addWidget(new SettingsCard(
        ":/svg/dark/settings.svg",
        tr("Remote Focus Navigation"),
        tr("Direction and Select keys automatically show focus across browsing views; pointer input hides it"),
        nullptr,
        QString(),
        this));

    m_mainLayout->addStretch();
}
