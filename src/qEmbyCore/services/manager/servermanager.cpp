#include "servermanager.h"
#include "../../api/embywebsocket.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QDir>

ServerManager::ServerManager(NetworkManager* nm, QObject* parent)
    : ServerManager(nm, QString(), parent) {
}

ServerManager::ServerManager(NetworkManager* nm,
                             const QString& settingsFilePath,
                             QObject* parent)
    : QObject(parent), m_network(nm), m_settingsFilePath(settingsFilePath) {
    loadSettings();
}

void ServerManager::addServer(const ServerProfile& profile) {
    
    for (int i = 0; i < m_servers.size(); ++i) {
        if (m_servers[i].url == profile.url && m_servers[i].userId == profile.userId) {
            m_servers[i] = profile;
            saveSettings();
            Q_EMIT serversChanged();
            return;
        }
    }
    m_servers.append(profile);
    saveSettings();
    Q_EMIT serversChanged();
}

void ServerManager::removeServer(const QString& id) {
    for (int i = 0; i < m_servers.size(); ++i) {
        if (m_servers[i].id == id) {
            
            m_servers.removeAt(i);

            
            saveSettings();
            Q_EMIT serversChanged();

            
            if (m_activeProfile.id == id) {
                
                disconnectWebSocket();
                
                m_activeProfile = ServerProfile();
                m_activeClient.reset();

                
                if (!m_servers.isEmpty()) {
                    setActiveServer(m_servers.first().id);
                } else {
                    
                    Q_EMIT activeServerChanged(m_activeProfile);
                }
            }
            break; 
        }
    }
}

void ServerManager::setActiveServer(const QString& id) {
    for (const auto& profile : m_servers) {
        if (profile.id == id) {
            m_activeProfile = profile;
            
            m_activeClient = QSharedPointer<ApiClient>::create(profile, m_network);
            Q_EMIT activeServerChanged(m_activeProfile);
            return;
        }
    }
}

void ServerManager::updateServerProxy(const QString& id,
                                      const ProxyConfig& proxy,
                                      bool useGlobalProxy) {
    bool found = false;
    bool isActive = false;
    for (int i = 0; i < m_servers.size(); ++i) {
        if (m_servers[i].id != id) {
            continue;
        }
        
        if (m_servers[i].proxy == proxy &&
            m_servers[i].useGlobalProxy == useGlobalProxy) {
            qDebug() << "[ServerManager] updateServerProxy skipped: no change"
                     << "| id:" << id;
            return;
        }
        m_servers[i].proxy = proxy;
        m_servers[i].useGlobalProxy = useGlobalProxy;

        
        
        if (m_activeProfile.id == id) {
            m_activeProfile = m_servers[i];
            isActive = true;
        }
        found = true;
        break;
    }

    if (!found) {
        qWarning() << "[ServerManager] updateServerProxy: server not found"
                   << "| id:" << id;
        return;
    }

    saveSettings();
    qInfo() << "[ServerManager] proxy updated"
            << "| id:" << id
            << "| useGlobalProxy:" << useGlobalProxy
            << "| proxy:" << proxy.summary();

    Q_EMIT serversChanged();
    Q_EMIT serverProxyChanged(id);
    if (isActive) {
        Q_EMIT activeServerChanged(m_activeProfile);
    }
}





void ServerManager::connectWebSocket()
{
    
    if (m_activeProfile.url.isEmpty() || m_activeProfile.accessToken.isEmpty()) return;

    
    if (m_activeWebSocket && m_activeWebSocket->isConnected()) return;

    
    if (m_activeWebSocket) {
        m_activeWebSocket->disconnectFromServer();
        m_activeWebSocket->deleteLater();
        m_activeWebSocket = nullptr;
    }

    
    m_activeWebSocket = new EmbyWebSocket(m_activeProfile, this);
    m_activeWebSocket->connectToServer();
}

void ServerManager::disconnectWebSocket()
{
    if (m_activeWebSocket) {
        m_activeWebSocket->disconnectFromServer();
        m_activeWebSocket->deleteLater();
        m_activeWebSocket = nullptr;
    }
}

EmbyWebSocket* ServerManager::activeWebSocket() const
{
    return m_activeWebSocket;
}





bool ServerManager::saveSettings(QString* errorString) {
    auto reportFailure = [this, errorString](const QString& message) {
        if (errorString) {
            *errorString = message;
        }
        qWarning() << "[ServerManager] Failed to save server settings"
                   << "| error:" << message;
        Q_EMIT settingsSaveFailed(message);
        return false;
    };

    
    QJsonArray array;
    for (const auto& p : m_servers) {
        QJsonObject obj;
        obj["id"] = p.id;
        obj["name"] = p.name;
        obj["url"] = p.url;
        obj["type"] = (p.type == ServerProfile::Emby ? "Emby" : "Jellyfin");
        obj["ignoreSslVerification"] = p.ignoreSslVerification;
        obj["userId"] = p.userId;
        obj["userName"] = p.userName;
        obj["accessToken"] = p.accessToken;
        obj["deviceId"] = p.deviceId;
        obj["isAdmin"] = p.isAdmin;
        obj["canDownloadMedia"] = p.canDownloadMedia;
        obj["iconBase64"] = p.iconBase64;
        obj["useGlobalProxy"] = p.useGlobalProxy;
        obj["proxy"] = p.proxy.toJson();
        array.append(obj);
    }

    QString filePath = m_settingsFilePath;
    if (filePath.isEmpty()) {
        const QString appDataPath =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (appDataPath.isEmpty()) {
            return reportFailure(tr("应用数据目录不可用。"));
        }
        filePath = QDir(appDataPath).filePath(QStringLiteral("servers.json"));
    }
    const QString path = QFileInfo(filePath).absolutePath();
    if (path.isEmpty()) {
        return reportFailure(tr("应用数据目录不可用。"));
    }
    if (!QDir().mkpath(path)) {
        return reportFailure(tr("无法创建应用数据目录：%1").arg(path));
    }

    QSaveFile file(filePath);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) {
        return reportFailure(file.errorString());
    }

    const QByteArray data = QJsonDocument(array).toJson();
    if (file.write(data) != data.size()) {
        const QString message = file.errorString().isEmpty()
            ? tr("服务器配置写入不完整。")
            : file.errorString();
        file.cancelWriting();
        return reportFailure(message);
    }
    if (!file.commit()) {
        return reportFailure(file.errorString());
    }

    if (errorString) {
        errorString->clear();
    }
    return true;
}

void ServerManager::loadSettings() {
    QString filePath = m_settingsFilePath;
    if (filePath.isEmpty()) {
        const QString appDataPath =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (appDataPath.isEmpty()) {
            return;
        }
        filePath = QDir(appDataPath).filePath(QStringLiteral("servers.json"));
    }
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
    m_servers.clear();
    for (auto val : array) {
        QJsonObject obj = val.toObject();
        ServerProfile p;
        p.id = obj["id"].toString();
        p.name = obj["name"].toString();
        p.url = obj["url"].toString();
        p.type = (obj["type"].toString() == "Emby" ? ServerProfile::Emby : ServerProfile::Jellyfin);
        p.ignoreSslVerification = obj["ignoreSslVerification"].toBool(false);
        p.userId = obj["userId"].toString();
        p.userName = obj["userName"].toString();
        p.accessToken = obj["accessToken"].toString();
        p.deviceId = obj["deviceId"].toString();
        p.isAdmin = obj["isAdmin"].toBool();
        p.canDownloadMedia = obj["canDownloadMedia"].toBool(false);
        p.iconBase64 = obj["iconBase64"].toString();
        p.useGlobalProxy = obj["useGlobalProxy"].toBool(false);
        p.proxy = ProxyConfig::fromJson(obj["proxy"].toObject());
        m_servers.append(p);
    }

    
    if (!m_servers.isEmpty()) {
        setActiveServer(m_servers.first().id);
    }
}

void ServerManager::clearActiveSession()
{
    disconnectWebSocket();
    m_activeProfile = ServerProfile();
    m_activeClient.reset();
    Q_EMIT activeServerChanged(m_activeProfile);
}
