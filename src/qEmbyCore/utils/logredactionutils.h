#ifndef LOGREDACTIONUTILS_H
#define LOGREDACTIONUTILS_H

#include "../qEmbyCore_global.h"

#include <QString>
#include <QStringList>
#include <QUrl>

class QEMBYCORE_EXPORT LogRedactionUtils
{
public:
    static QString url(const QUrl &url);
    static QString url(const QString &value);
    static QString text(const QString &value);
    static QString proxy(const QString &value);
    static QStringList stringList(const QStringList &values);
};

#endif
