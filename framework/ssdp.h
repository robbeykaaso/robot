#ifndef REAL_APPLICATION2_SSDP_H_
#define REAL_APPLICATION2_SSDP_H_

#include <QObject>
#include <QUdpSocket>
#include <memory>
#include "util.h"

namespace dst {

class DSTDLL DiscoveryManager : public QObject
{
    Q_OBJECT
public:
    DiscoveryManager(quint16 aReceivePort = 8000, QObject *parent = nullptr );
    void StartDiscovery(const std::string& aST = "ds:training_server");
//    void SendAlive();
signals:
    void FoundServer(QString aIP, QString aPort, QString aID);
public slots:
    void ReadPending();
private:
    std::shared_ptr<QUdpSocket> sock_;
private:
    const quint16 boardcast_port = 1900;
    const QHostAddress groupAddress =QHostAddress("239.255.255.250");//  QHostAddress::Broadcast;
};

}

#endif
