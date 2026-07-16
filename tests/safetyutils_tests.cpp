#include <utils/logredactionutils.h>
#include "../src/qEmbyApp/utils/asyncrequestgate.h"

#include <QTest>
#include <QUrlQuery>

class SafetyUtilsTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void redactsSensitiveQueryValuesAndCredentials()
    {
        const QString value = QStringLiteral(
            "https://user:password@example.test/Items?api_key=secret&Limit=25&refreshToken=encoded%20secret");
        const QUrl redacted(LogRedactionUtils::url(value));
        const QUrlQuery query(redacted);

        QCOMPARE(redacted.userName(QUrl::FullyDecoded), QStringLiteral("***"));
        QVERIFY(redacted.password().isEmpty());
        QCOMPARE(query.queryItemValue(QStringLiteral("api_key"), QUrl::FullyDecoded),
                 QStringLiteral("***"));
        QCOMPARE(query.queryItemValue(QStringLiteral("refreshToken"), QUrl::FullyDecoded),
                 QStringLiteral("***"));
        QCOMPARE(query.queryItemValue(QStringLiteral("Limit"), QUrl::FullyDecoded),
                 QStringLiteral("25"));
        QCOMPARE(redacted.host(), QStringLiteral("example.test"));
        QCOMPARE(redacted.path(), QStringLiteral("/Items"));
    }

    void redactionIsCaseInsensitiveAndPreservesOrdinaryUrls()
    {
        const QUrl source(QStringLiteral(
            "wss://example.test/socket?ACCESS_TOKEN=abc&api%5Ftoken=def&deviceId=device-1"));
        const QUrl redacted(LogRedactionUtils::url(source));
        const QUrlQuery query(redacted);

        QCOMPARE(query.queryItemValue(QStringLiteral("ACCESS_TOKEN"), QUrl::FullyDecoded),
                 QStringLiteral("***"));
        QCOMPARE(query.queryItemValue(QStringLiteral("api_token"), QUrl::FullyDecoded),
                 QStringLiteral("***"));
        QCOMPARE(query.queryItemValue(QStringLiteral("deviceId"), QUrl::FullyDecoded),
                 QStringLiteral("device-1"));

        const QString ordinary = QStringLiteral(
            "https://example.test/library?Limit=10&SortBy=Name");
        QCOMPARE(LogRedactionUtils::url(ordinary), ordinary);
    }

    void requestGateRejectsOlderGenerationsAndContexts()
    {
        AsyncRequestGate gate;
        const auto first = gate.begin(QStringLiteral("server-a|user-a"));
        QVERIFY(gate.isCurrent(first, QStringLiteral("server-a|user-a")));

        const auto second = gate.begin(QStringLiteral("server-a|user-a"));
        QVERIFY(!gate.isCurrent(first, QStringLiteral("server-a|user-a")));
        QVERIFY(gate.isCurrent(second, QStringLiteral("server-a|user-a")));

        gate.invalidate(QStringLiteral("server-b|user-b"));
        QVERIFY(!gate.isCurrent(second, QStringLiteral("server-a|user-a")));

        const auto third = gate.begin(QStringLiteral("server-b|user-b"));
        QVERIFY(gate.isCurrent(third, QStringLiteral("server-b|user-b")));
        QVERIFY(!gate.isCurrent(third, QStringLiteral("server-b|user-c")));
    }

    void redactsUrlsAndStandaloneTokensInsideDiagnosticText()
    {
        const QString diagnostic = QStringLiteral(
            "Request failed for https://user:password@example.test/video?api_key=secret&Limit=2 "
            "access_token=standalone");
        const QString redacted = LogRedactionUtils::text(diagnostic);

        QVERIFY(!redacted.contains(QStringLiteral("password")));
        QVERIFY(!redacted.contains(QStringLiteral("secret")));
        QVERIFY(!redacted.contains(QStringLiteral("standalone")));
        QVERIFY(redacted.contains(QStringLiteral("example.test/video")));
        QVERIFY(redacted.contains(QStringLiteral("Limit=2")));
    }
};

QTEST_GUILESS_MAIN(SafetyUtilsTests)
#include "safetyutils_tests.moc"
