#include "testserver.h"
#include <QJsonDocument>
//#include "aws_s3.h"

TestServer::TestServer(QObject *parent) : QObject(parent)
{
    auto tmp = socket_.listen(QHostAddress::LocalHost, 8082);
    connect(&socket_,SIGNAL(newConnection()),this,SLOT(NewConnect()));
}

void TestServer::NewConnect()
{
    client_ = socket_.nextPendingConnection(); //得到每个连进来的socket
    connect(client_,SIGNAL(readyRead()),this,SLOT(ReadMessage())); //有可读的信息，触发读函数槽
    initialize();
    /*QJsonObject log;
    log.insert("type", "task_log");
    log.insert("project_id", "test_project");
    log.insert("task_id", "test_task");
    log.insert("job_id", "test_job");

    log.insert("log_msg", "Server is connected");
    QJsonDocument doc(log);
    QString msg(doc.toJson(QJsonDocument::Compact));
    (msg.toStdString());*/
}

void uploadJobResult(const QString& aFolder, const QString& aBucketName, const QString& aSuffix, const QString& aPrefix = ""){
/*    AWSClient* aws_ = reinterpret_cast<AWSClient*>(Storage::instance()->GetSomething());
    const std::shared_ptr<Aws::IOStream> input_data =
        Aws::MakeShared<Aws::FStream>("SampleAllocationTag",
                                      ("../deepinspectGUI/test/" + aPrefix + "result" + aSuffix +".json").toStdString(),
                                      std::ios_base::in | std::ios_base::binary);
    aws_->put_s3_string_object(aBucketName.toStdString().c_str(), (aFolder + "result.json").toStdString().c_str(), input_data);*/
/*    const std::shared_ptr<Aws::IOStream> input_data2 =
        Aws::MakeShared<Aws::FStream>("SampleAllocationTag",
                                      ("../deepinspectGUI/test/sample" + aPrefix + ".json").toStdString(),
                                      std::ios_base::in | std::ios_base::binary);
    aws_->put_s3_string_object(aBucketName.toStdString().c_str(), (aFolder + "annotations/EDD033BE-F0E5-49ab-AF8B-065B5004E857.bmp.json").toStdString().c_str(), input_data2);*/
}

void TestServer::TryResponseClient(const QJsonObject& aRequest)
{
    static QSet<QString> jobs;
    QJsonObject response;
    response.insert("id", aRequest.value("id"));
    response.insert("type", aRequest.value("type"));
    if (aRequest.value("type") == "connect"){
        if (aRequest.value("alias") != "")
            response.insert("err_code", 0);
        else
            response.insert("err_code", 1);
        response.insert("msg", "response message");
    }else if (aRequest.value("type") == "display_params"){
        //uploadTaskPanel(aRequest.value("display_json_folder").toString() + "/TaskPanel.qml", aRequest.value("s3_bucket_name").toString());
        response.insert("err_code", 0);
        response.insert("msg", "");
    }else if (aRequest.value("type") == "task_state"){
        dst::streamManager::instance()->emitSignal(aRequest.value("type").toString(), STMJSON(aRequest));
        return;
    }else if (aRequest.value("type") == "training"){
        dst::streamManager::instance()->emitSignal(aRequest.value("type").toString(), STMJSON(aRequest));
        return;
    }else if (aRequest.value("type") == "inference"){
        dst::streamManager::instance()->emitSignal(aRequest.value("type").toString(), STMJSON(aRequest));
        return;
    }else if (aRequest.value("type") == "upload"){
        dst::streamManager::instance()->emitSignal(aRequest.value("type").toString(), STMJSON(aRequest));
        return;
    }else if (aRequest.value("type") == "general_state"){
        QJsonObject user;
        QJsonObject token;
        QJsonObject proj;
        QJsonObject tsk;
        QJsonObject job;
        job.insert("stage", "training");
        job.insert("state", "running");
        tsk.insert("name", "hello_task1");
        tsk.insert("job_id_1", job);
        tsk.insert("job_id_2", job);
        proj.insert("task_id_1", tsk);
        job.insert("stage", "download");
        job.insert("state", "finish");
        tsk.insert("name", "hello_task2");
        tsk.insert("job_id_1", job);
        tsk.insert("job_id_2", job);
        proj.insert("task_id_2", tsk);
        tsk.insert("job_id", "job2");
        proj.insert("name", "hello_project1");
        token.insert("project1", proj);
        token.insert("alias", "hello");
        user.insert("token1", token);
        response.insert("state", user);
        response.insert("err_code", 0);
        response.insert("mgs", "response message");
    }else if (aRequest.value("type") == "delete" || aRequest.value("type") == "stop_job"){
        response.insert("relative_running_task_list", "");
        response.insert("err_code", 0);
        response.insert("mgs", "response message");
    }else if (aRequest.value("type") == "recommend_params"){
        auto prm = aRequest.value("params").toObject();
        auto preprocess = prm.value("preprocess").toObject();
        preprocess.insert("min_feature_size", 15);
        prm.insert("preprocess", preprocess);
        response.insert("params", prm);
        response.insert("err_code", 0);
        response.insert("mgs", "response message");
    }
    QJsonDocument doc;
    doc.setObject(response);
    QString msg(doc.toJson(QJsonDocument::Compact));
    Send(msg.toStdString());

    /*QJsonObject log_test;
    log_test.insert("type", "task_log");
    log_test.insert("project_id", m_project_id);
    log_test.insert("task_id", m_task_id);
    log_test.insert("log_level", "info");
    log_test.insert("log_msg", "train tick: " + QString::number(m_train_ticks));
    doc.setObject(log_test);
    QString strJson(doc.toJson(QJsonDocument::Compact));
    Send(strJson.toStdString());

    QJsonObject progress_test;
    progress_test.insert("type", "task_progress_push");
    progress_test.insert("project_id", m_project_id);
    progress_test.insert("task_id", m_task_id);
    if (m_train_ticks % 10 == 0)
        progress_test.insert("progress", 100);
    else
        progress_test.insert("progress", (m_train_ticks % 10) * 10);
    progress_test.insert("progress_msg", "remain time: about 2 minute");
    doc.setObject(progress_test);
    QString strJson2(doc.toJson(QJsonDocument::Compact));
    Send(strJson2.toStdString());*/
}

void TestServer::ReadMessage()	//读取信息
{
    if (!client_)
        return;    
    QByteArray qba= client_->readAll(); //读取
    QString ss=QVariant(qba).toString();
    auto strs = dst::parseJsons(ss);
    for (auto str : strs){
        QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8());
        TryResponseClient(doc.object());
    }
}


void TestServer::Send(const std::string& aContent) //发送信息
{
    if (client_){
        client_->write(aContent.c_str()); //发送
        client_->flush();
        while (client_->bytesToWrite() > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
