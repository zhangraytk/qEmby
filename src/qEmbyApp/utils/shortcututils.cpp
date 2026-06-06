#include "shortcututils.h"

#include "config/config_keys.h"
#include "config/configstore.h"
#include <QKeyEvent>
#include <QRegularExpression>
#include <QSet>

namespace
{

int keyAlias(const QString& token)
{
    QString key = token.simplified().toLower();
    key.remove(QChar(' '));

    if (key == QStringLiteral("esc") || key == QStringLiteral("escape")) {
        return Qt::Key_Escape;
    }
    if (key == QStringLiteral("back")) {
        return Qt::Key_Back;
    }
    if (key == QStringLiteral("backspace") || key == QStringLiteral("bs")) {
        return Qt::Key_Backspace;
    }
    if (key == QStringLiteral("pgup") || key == QStringLiteral("pageup")) {
        return Qt::Key_PageUp;
    }
    if (key == QStringLiteral("pgdown") || key == QStringLiteral("pagedown")) {
        return Qt::Key_PageDown;
    }
    if (key == QStringLiteral("left")) {
        return Qt::Key_Left;
    }
    if (key == QStringLiteral("right")) {
        return Qt::Key_Right;
    }
    if (key == QStringLiteral("up")) {
        return Qt::Key_Up;
    }
    if (key == QStringLiteral("down")) {
        return Qt::Key_Down;
    }
    if (key == QStringLiteral("home")) {
        return Qt::Key_Home;
    }
    if (key == QStringLiteral("end")) {
        return Qt::Key_End;
    }
    if (key == QStringLiteral("space")) {
        return Qt::Key_Space;
    }
    if (key == QStringLiteral("enter") || key == QStringLiteral("return")) {
        return Qt::Key_Return;
    }
    if (key == QStringLiteral("select")) {
        return Qt::Key_Select;
    }
    if (key == QStringLiteral("play")) {
        return Qt::Key_Play;
    }
    if (key == QStringLiteral("mediaplay")) {
        return Qt::Key_MediaPlay;
    }
    if (key == QStringLiteral("mediapause")) {
        return Qt::Key_MediaPause;
    }
    if (key == QStringLiteral("playpause") ||
        key == QStringLiteral("mediaplaypause") ||
        key == QStringLiteral("mediatoggleplaypause")) {
        return Qt::Key_MediaTogglePlayPause;
    }
    if (key == QStringLiteral("del") || key == QStringLiteral("delete")) {
        return Qt::Key_Delete;
    }

    return 0;
}

bool parseAliasSequence(const QString& text, QKeySequence* out)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    Qt::KeyboardModifiers modifiers;
    int key = 0;
    const QStringList tokens = trimmed.split(QStringLiteral("+"),
                                             Qt::SkipEmptyParts);
    for (QString token : tokens) {
        token = token.trimmed();
        const QString lowered = token.simplified().toLower();
        if (lowered == QStringLiteral("ctrl") ||
            lowered == QStringLiteral("control")) {
            modifiers |= Qt::ControlModifier;
            continue;
        }
        if (lowered == QStringLiteral("alt") ||
            lowered == QStringLiteral("option")) {
            modifiers |= Qt::AltModifier;
            continue;
        }
        if (lowered == QStringLiteral("shift")) {
            modifiers |= Qt::ShiftModifier;
            continue;
        }
        if (lowered == QStringLiteral("meta") ||
            lowered == QStringLiteral("cmd") ||
            lowered == QStringLiteral("command") ||
            lowered == QStringLiteral("super")) {
            modifiers |= Qt::MetaModifier;
            continue;
        }

        const int aliasKey = keyAlias(token);
        if (aliasKey == 0 || key != 0) {
            return false;
        }
        key = aliasKey;
    }

    if (key == 0) {
        return false;
    }

    *out = QKeySequence(modifiers.toInt() | key);
    return true;
}

} 

namespace ShortcutUtils
{

QKeySequence fromKeyEvent(QKeyEvent* event)
{
    if (!event || event->key() == Qt::Key_unknown) {
        return {};
    }

    const int key = event->key();
    Qt::KeyboardModifiers modifiers = event->modifiers();
    modifiers &= ~Qt::KeypadModifier;

    if (key == Qt::Key_Shift || key == Qt::Key_Control ||
        key == Qt::Key_Meta || key == Qt::Key_Alt ||
        key == Qt::Key_AltGr) {
        return {};
    }

    return QKeySequence(modifiers.toInt() | key);
}

QKeySequence fromUserString(const QString& value)
{
    QKeySequence aliasSequence;
    if (parseAliasSequence(value, &aliasSequence)) {
        return aliasSequence;
    }

    return QKeySequence::fromString(value.trimmed(),
                                    QKeySequence::PortableText);
}

QStringList splitShortcutList(const QString& value)
{
    QStringList parts;
    const QStringList rawParts =
        value.split(QRegularExpression(QStringLiteral("[;\\n]")),
                    Qt::SkipEmptyParts);
    for (QString part : rawParts) {
        part = part.trimmed();
        if (!part.isEmpty()) {
            parts.append(part);
        }
    }
    return parts;
}

bool matchesShortcutList(const QKeySequence& sequence, const QString& value)
{
    if (sequence.isEmpty()) {
        return false;
    }

    for (const QString& part : splitShortcutList(value)) {
        const QKeySequence configured = fromUserString(part);
        if (!configured.isEmpty() &&
            sequence.matches(configured) == QKeySequence::ExactMatch) {
            return true;
        }
    }

    return false;
}

QString defaultNavigationBackShortcuts()
{
    QStringList shortcuts;
    QSet<QString> seen;

    auto append = [&shortcuts, &seen](const QString& value) {
        const QString trimmed = value.trimmed();
        if (trimmed.isEmpty() || seen.contains(trimmed)) {
            return;
        }
        seen.insert(trimmed);
        shortcuts.append(trimmed);
    };

    const auto systemBack = QKeySequence::keyBindings(QKeySequence::Back);
    for (const QKeySequence& sequence : systemBack) {
        append(sequence.toString(QKeySequence::PortableText));
    }

#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    append(QStringLiteral("Meta+Left"));
    append(QStringLiteral("Meta+["));
#else
    append(QStringLiteral("Alt+Left"));
#endif
    append(QStringLiteral("Back"));
    append(QStringLiteral("Esc"));

    return shortcuts.join(QStringLiteral("; "));
}

QString defaultNavigationHomeShortcut()
{
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    return QStringLiteral("Meta+Shift+H");
#else
    return QStringLiteral("Ctrl+H");
#endif
}

QString defaultNavigationFavoritesShortcut()
{
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    return QStringLiteral("Meta+Shift+F");
#else
    return QStringLiteral("Ctrl+Shift+F");
#endif
}

QString defaultFeedPreviousPageShortcuts()
{
    return QStringLiteral("PgUp; Page Up");
}

QString defaultFeedNextPageShortcuts()
{
    return QStringLiteral("PgDown; Page Down");
}

void migrateLegacyShortcutDefaults()
{
    auto* store = ConfigStore::instance();
    const QStringList keys = store->allKeys();

    auto migrateIfLegacy = [store, &keys](const QString& key,
                                          const QString& legacy,
                                          const QString& replacement) {
        if (!keys.contains(key)) {
            return;
        }

        const QString current = store->get<QString>(key).trimmed();
        if (current == legacy && current != replacement) {
            store->set(key, replacement);
        }
    };

    migrateIfLegacy(ConfigKeys::ShortcutNavigationBack,
                    QStringLiteral("Back; Alt+Left; Esc; Escape"),
                    defaultNavigationBackShortcuts());
    migrateIfLegacy(ConfigKeys::ShortcutNavigationHome,
                    QStringLiteral("Ctrl+H"),
                    defaultNavigationHomeShortcut());
    migrateIfLegacy(ConfigKeys::ShortcutNavigationFavorites,
                    QStringLiteral("Ctrl+Shift+F"),
                    defaultNavigationFavoritesShortcut());
    migrateIfLegacy(ConfigKeys::ShortcutFeedPreviousPage,
                    QStringLiteral("PgUp; Page Up"),
                    defaultFeedPreviousPageShortcuts());
    migrateIfLegacy(ConfigKeys::ShortcutFeedPreviousPage,
                    QStringLiteral("Left; PgUp; Page Up"),
                    defaultFeedPreviousPageShortcuts());
    migrateIfLegacy(ConfigKeys::ShortcutFeedNextPage,
                    QStringLiteral("PgDown; Page Down"),
                    defaultFeedNextPageShortcuts());
    migrateIfLegacy(ConfigKeys::ShortcutFeedNextPage,
                    QStringLiteral("Right; PgDown; Page Down"),
                    defaultFeedNextPageShortcuts());
}

}
