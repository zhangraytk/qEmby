#include "logredactionutils.h"

#include <QRegularExpression>
#include <QUrlQuery>

namespace {

bool isSensitiveQueryKey(const QString &key)
{
    return key.compare(QStringLiteral("api_key"), Qt::CaseInsensitive) == 0 ||
           key.compare(QStringLiteral("api_token"), Qt::CaseInsensitive) == 0 ||
           key.compare(QStringLiteral("access_token"), Qt::CaseInsensitive) == 0 ||
           key.compare(QStringLiteral("X-Emby-Token"), Qt::CaseInsensitive) == 0 ||
           key.contains(QStringLiteral("token"), Qt::CaseInsensitive);
}

QString redactUrl(QUrl redacted)
{
    if (!redacted.userName().isEmpty() || !redacted.password().isEmpty()) {
        redacted.setUserName(QStringLiteral("***"));
        redacted.setPassword(QString());
    }

    QUrlQuery query(redacted);
    if (!query.isEmpty()) {
        QUrlQuery cleanQuery;
        const auto items = query.queryItems(QUrl::FullyDecoded);
        for (const auto &item : items) {
            cleanQuery.addQueryItem(
                item.first,
                isSensitiveQueryKey(item.first) ? QStringLiteral("***") : item.second);
        }
        redacted.setQuery(cleanQuery);
    }

    return redacted.toString(QUrl::FullyEncoded | QUrl::RemovePassword);
}

}

QString LogRedactionUtils::url(const QUrl &url)
{
    if (!url.isValid() || url.isEmpty()) {
        return url.toString();
    }
    return redactUrl(url);
}

QString LogRedactionUtils::url(const QString &value)
{
    if (!value.contains(QStringLiteral("://")) &&
        !value.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive)) {
        return value;
    }

    const QUrl parsed(value, QUrl::TolerantMode);
    if (!parsed.isValid() || parsed.scheme().isEmpty()) {
        return value;
    }
    return redactUrl(parsed);
}

QString LogRedactionUtils::text(const QString &value)
{
    QString redacted = value;

    static const QRegularExpression urlPattern(
        QStringLiteral(R"((?:[A-Za-z][A-Za-z0-9+.-]*://|file:)[^\s<>"']+)"));
    QList<QRegularExpressionMatch> urlMatches;
    auto urlIterator = urlPattern.globalMatch(redacted);
    while (urlIterator.hasNext()) {
        urlMatches.append(urlIterator.next());
    }
    for (auto it = urlMatches.crbegin(); it != urlMatches.crend(); ++it) {
        const QRegularExpressionMatch &match = *it;
        redacted.replace(match.capturedStart(), match.capturedLength(),
                         url(match.captured()));
    }

    static const QRegularExpression sensitivePairPattern(
        QStringLiteral(
            R"((\b(?:api[_-]?key|[A-Za-z0-9_.-]*token[A-Za-z0-9_.-]*)\b\s*[:=]\s*)([^\s&,;]+))"),
        QRegularExpression::CaseInsensitiveOption);
    QList<QRegularExpressionMatch> pairMatches;
    auto pairIterator = sensitivePairPattern.globalMatch(redacted);
    while (pairIterator.hasNext()) {
        pairMatches.append(pairIterator.next());
    }
    for (auto it = pairMatches.crbegin(); it != pairMatches.crend(); ++it) {
        const QRegularExpressionMatch &match = *it;
        const QString capturedValue = match.captured(2);
        QString replacementValue = QStringLiteral("***");
        if (capturedValue.size() >= 2 &&
            ((capturedValue.startsWith(QLatin1Char('"')) &&
              capturedValue.endsWith(QLatin1Char('"'))) ||
             (capturedValue.startsWith(QLatin1Char('\'')) &&
              capturedValue.endsWith(QLatin1Char('\''))))) {
            replacementValue = capturedValue.left(1) + QStringLiteral("***") +
                               capturedValue.right(1);
        }
        redacted.replace(match.capturedStart(), match.capturedLength(),
                         match.captured(1) + replacementValue);
    }

    return redacted;
}

QString LogRedactionUtils::proxy(const QString &value)
{
    if (value.isEmpty()) {
        return QStringLiteral("<empty>");
    }

    const QUrl parsed(value, QUrl::TolerantMode);
    if (!parsed.isValid() || parsed.host().isEmpty()) {
        return value;
    }
    return redactUrl(parsed);
}

QStringList LogRedactionUtils::stringList(const QStringList &values)
{
    QStringList redacted;
    redacted.reserve(values.size());
    for (const QString &value : values) {
        redacted.append(url(value));
    }
    return redacted;
}
