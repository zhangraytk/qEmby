#include "shortcututils.h"

#include <QKeyEvent>
#include <QRegularExpression>

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

}
