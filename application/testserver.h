#ifndef REAL_APPLICATION2_TESTSERVER_H_
#define REAL_APPLICATION2_TESTSERVER_H_

#include "../framework/rxmanager.h"
#include <QTcpServer>
#include <QTcpSocket>

class TestServer : QObject{
    Q_OBJECT
public:
    void Send(const std::string& aContent);
public slots:
    void NewConnect();
    void ReadMessage();
protected:
    TestServer(QObject *parent = nullptr);
    virtual void initialize(){}
private:
    void TryResponseClient(const QJsonObject& aRequest);
    QTcpServer socket_;
    QTcpSocket* client_;
};

#endif
