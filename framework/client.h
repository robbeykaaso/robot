#ifndef REAL_APPLICATION2_CLIENTSOCKET_H_
#define REAL_APPLICATION2_CLIENTSOCKET_H_

#include "../framework/ssdp.h"
#include "../framework/document.h"
#include <QTcpSocket>
#include <QTimer>

class Client : public dst::configObject{
    Q_OBJECT
public:
    Client(const QJsonObject& aConfig);
    ~Client();
    void RegistOnStateChanged(std::function<void(QAbstractSocket::SocketState)> aEvent);
    bool isValid() {return valid_;}
signals:
    void connected();
public slots:
    void ServerFound(QString aIP, QString aPort, QString aID);
    void ReceiveState(QAbstractSocket::SocketState aState);
    void ReceiveMessage();
private:
    void tryConnectServer();
    bool isValidServer(QString aIP, QString aPort);
    QTcpSocket socket_;
    dst::DiscoveryManager ssdp_;
    bool valid_ = false;
    QTimer search_timer_;
    QMap<QString, std::shared_ptr<dst::streamData>> on_listened_;
};

#endif
