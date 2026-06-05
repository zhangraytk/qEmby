#include "shortcutedit.h"

#include "../utils/shortcututils.h"
#include <QKeyEvent>
#include <QTimer>

ShortcutEdit::ShortcutEdit(QWidget* parent)
    : QLineEdit(parent)
{
    setObjectName(QStringLiteral("ShortcutEdit"));
    setClearButtonEnabled(true);
    setPlaceholderText(tr("Click and press a shortcut"));
}

void ShortcutEdit::focusInEvent(QFocusEvent* event)
{
    QLineEdit::focusInEvent(event);
    QTimer::singleShot(0, this, [this]() { selectAll(); });
}

void ShortcutEdit::keyPressEvent(QKeyEvent* event)
{
    if (!event) {
        return;
    }

    if (event->matches(QKeySequence::Paste)) {
        QLineEdit::keyPressEvent(event);
        Q_EMIT shortcutTextChanged(text().trimmed());
        return;
    }

    if ((event->key() == Qt::Key_Backspace ||
         event->key() == Qt::Key_Delete) &&
        event->modifiers() == Qt::NoModifier) {
        clear();
        Q_EMIT shortcutTextChanged(QString());
        event->accept();
        return;
    }

    const QKeySequence sequence = ShortcutUtils::fromKeyEvent(event);
    if (sequence.isEmpty()) {
        event->ignore();
        return;
    }

    const QString portable = sequence.toString(QKeySequence::PortableText);
    if (portable.isEmpty()) {
        event->ignore();
        return;
    }

    setText(portable);
    selectAll();
    Q_EMIT shortcutTextChanged(portable);
    event->accept();
}
