#ifndef SHORTCUTUTILS_H
#define SHORTCUTUTILS_H

#include <QKeySequence>
#include <QString>
#include <QStringList>

class QKeyEvent;

namespace ShortcutUtils
{
QKeySequence fromKeyEvent(QKeyEvent* event);
QKeySequence fromUserString(const QString& value);
QStringList splitShortcutList(const QString& value);
bool matchesShortcutList(const QKeySequence& sequence, const QString& value);
}

#endif
