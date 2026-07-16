#include <api/networkmanager.h>
#include <models/profile/serverprofile.h>
#include <services/manager/servermanager.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTest>
#include <QTemporaryDir>

class ServerManagerTests : public QObject
{
    Q_OBJECT

private:
    ServerProfile profile(const QString& id) const
    {
        ServerProfile result;
        result.id = id;
        result.name = QStringLiteral("Server ") + id;
        result.url = QStringLiteral("https://example.test/") + id;
        result.userId = QStringLiteral("user-") + id;
        result.userName = QStringLiteral("User ") + id;
        result.accessToken = QStringLiteral("token-") + id;
        result.deviceId = QStringLiteral("device-") + id;
        return result;
    }

private Q_SLOTS:
    void atomicallySavesAndReloadsProfiles()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        const QString settingsPath =
            temporaryDir.filePath(QStringLiteral("servers.json"));
        NetworkManager network;
        ServerManager manager(&network, settingsPath);
        manager.addServer(profile(QStringLiteral("one")));

        QFile file(settingsPath);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
        QCOMPARE(error.error, QJsonParseError::NoError);
        QVERIFY(document.isArray());
        QCOMPARE(document.array().size(), 1);

        ServerManager reloaded(&network, settingsPath);
        QCOMPARE(reloaded.servers().size(), 1);
        QCOMPARE(reloaded.servers().first().id, QStringLiteral("one"));
        QCOMPARE(reloaded.servers().first().accessToken,
                 QStringLiteral("token-one"));
    }

    void failedSaveSignalsAndPreservesExistingFile()
    {
#if defined(Q_OS_WIN)
        QSKIP("Directory permission failure is not deterministic on Windows.");
#else
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        const QString settingsPath =
            temporaryDir.filePath(QStringLiteral("servers.json"));
        NetworkManager network;
        ServerManager manager(&network, settingsPath);
        manager.addServer(profile(QStringLiteral("one")));

        QFile originalFile(settingsPath);
        QVERIFY(originalFile.open(QIODevice::ReadOnly));
        const QByteArray original = originalFile.readAll();
        originalFile.close();

        const QFileDevice::Permissions originalPermissions =
            QFileInfo(temporaryDir.path()).permissions();
        QVERIFY(QFile::setPermissions(temporaryDir.path(),
                                      QFileDevice::ReadOwner |
                                          QFileDevice::ExeOwner));

        QSignalSpy failures(&manager, &ServerManager::settingsSaveFailed);
        manager.addServer(profile(QStringLiteral("two")));

        QVERIFY(QFile::setPermissions(temporaryDir.path(), originalPermissions));
        QCOMPARE(failures.count(), 1);
        QVERIFY(!failures.first().first().toString().isEmpty());

        QFile preservedFile(settingsPath);
        QVERIFY(preservedFile.open(QIODevice::ReadOnly));
        QCOMPARE(preservedFile.readAll(), original);
        QCOMPARE(manager.servers().size(), 2);
#endif
    }
};

QTEST_GUILESS_MAIN(ServerManagerTests)
#include "servermanager_tests.moc"
