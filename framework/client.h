#ifndef REAL_APPLICATION2_CLIENTSOCKET_H_
#define REAL_APPLICATION2_CLIENTSOCKET_H_

#include "ssdp.h"
#include "document.h"
#include <QTcpSocket>
#include <QTimer>

class Client : public QObject{
    Q_OBJECT
public:
    SINGLENTON(Client)
public:
    ~Client();
    void RegistOnStateChanged(std::function<void(QAbstractSocket::SocketState)> aEvent);
    void Send(const std::string& aMessage, const QString& aID = "", std::function<void(const QJsonObject&)> aCallback = nullptr);
    bool isValid() {return valid_;}
signals:
    void connected();
public slots:
    void ServerFound(QString aIP, QString aPort, QString aID);
    void ReceiveState(QAbstractSocket::SocketState aState);
    void ReceiveMessage();
protected:
    Client(QObject *parent = nullptr);
private:
    void tryConnectServer();
    bool isValidServer(QString aIP, QString aPort);
    QTcpSocket socket_;
    DiscoveryManager ssdp_;
    bool valid_ = false;
    QTimer search_timer_;
    QMap<QString, std::function<void(const QJsonObject&)>> on_listened_;
};

void SendRequest(const QJsonObject& aParam, std::function<void(const QJsonObject&)> aCallback);
void updateGUIState(const QString& aCommand, const QJsonObject& aResponse);
QStringList parseJsons(const QString& aContent);

#endif
