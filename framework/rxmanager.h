#ifndef REAL_FRAMEWORK_RXMANAGER_H_
#define REAL_FRAMEWORK_RXMANAGER_H_

#define UNITTEST

#include <map>
#include <mutex>
#include <future>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QGuiApplication>
#include <QVector>
#include <QSet>
#include <QThread>
#include "util.h"

namespace dst {

/*template <typename T>
using onLambdaEvent = void*(T);

void* testfunc(std::function<void*(void*)> aLambda){

}*/

//singleton pattern：https://www.cnblogs.com/qiaoconglovelife/p/5851163.html
#define SINGLENTON(aName)  \
static aName* instance(){ \
static std::mutex aName##_mutex; \
std::lock_guard<std::mutex> lg(aName##_mutex); \
static aName ret; \
return &ret;}

DSTDLL QJsonObject Json(QJsonObject&& aTarget = QJsonObject());

template <typename T, typename S, typename ...Args>
QJsonObject Json(QJsonObject&& aTarget, T&& first, S&& second, Args&&... rest)
{
    auto ret = Json(std::forward<QJsonObject>(aTarget), std::forward<Args>(rest)...);
    ret.insert(first, second);
    return ret;
}

template <typename T, typename S, typename ...Args>
QJsonObject Json(T&& first, S&& second, Args&&... rest)
{
    auto ret = Json(QJsonObject(), std::forward<Args>(rest)...);
    ret.insert(first, second);
    return ret;
}

DSTDLL QJsonArray JArray(QJsonArray&& aTarget = QJsonArray());

template <typename T, typename ...Args>
QJsonArray JArray(QJsonArray&& aTarget, T&& first, Args&&... rest)
{
    auto ret = JArray(std::forward<QJsonArray>(aTarget), std::forward<Args>(rest)...);
    ret.push_front(first);
    return ret;
}

template <typename T, typename ...Args>
QJsonArray JArray(T&& first, Args&&... rest)
{
    auto ret = JArray(QJsonArray(), std::forward<Args>(rest)...);
    ret.push_front(first);
    return ret;
}

class DSTDLL streamData{
public:
    streamData(std::function<void(void*)> aCallback = nullptr) {
        m_callback = aCallback;
    }
    virtual ~streamData() {}
    void callback(void* aParam) {
        if (m_callback != nullptr)
            m_callback(aParam);
    }
    virtual std::shared_ptr<streamData> clone(){
        return std::make_shared<streamData>(m_callback);
    }
protected:
    std::function<void(void*)> m_callback;
};

class DSTDLL streamJson : public streamData{
public:
    streamJson(const QJsonObject& aData, std::function<void(void*)> aCallback = nullptr) : streamData(aCallback) {m_data = QJsonObject(aData); }
    void replaceData(const QJsonObject& aData) {m_data = aData;}
    QJsonObject* getData() {return &m_data;}
    std::shared_ptr<streamData> clone() override{
        return std::make_shared<streamJson>(m_data, m_callback);
    }
protected:
    QJsonObject m_data;
};

class streamManager;
using streamFunc = std::function<std::shared_ptr<streamData>(std::shared_ptr<streamData>)>;

class DSTDLL threadEvent : public QObject, std::enable_shared_from_this<threadEvent>{
public:
    threadEvent(const QString& aEventName = "");
    threadEvent(streamFunc aFunc, const QString& aEventName = "", int aThreadNo = 0);
    threadEvent(QJSValue aFunc, const QString& aEventName = "");
    ~threadEvent() override;
    virtual void execute(std::shared_ptr<streamData> aStream);
    threadEvent* previous(threadEvent* aPrevious, bool isPipe = false);
    threadEvent* next(threadEvent* aNext, bool isPipe = false);
    threadEvent* previous(const QString& aSignal, const QString& aEventName, bool isPipe = false);
    threadEvent* next(const QString& aSignal, const QString& aEventName, bool isPipe = false);
protected:
    void setName(const QString& aEventName);
    bool event( QEvent* e) override;
    QString m_name;
    streamFunc m_func;
    QJSValue m_jsfunc;
    QThread* m_thread = QThread::currentThread();
private:
    class streamEvent : public QEvent{
    public:
        static const Type type = static_cast<Type>(QEvent::User + 1);
    public:
        streamEvent(const QString& aName, std::shared_ptr<streamData> aStream) : QEvent(type) {
            m_name = aName;
            m_stream = aStream;
        }
        QString getName() {return m_name;}
        std::shared_ptr<streamData> getStream() {return m_stream;}
    private:
        QString m_name;
        std::shared_ptr<streamData> m_stream = nullptr;
    };
protected:
    QSet<QString> m_previous;
    QMap<QString, bool> m_next; //qset is unordered, but qmap is ordered
private:
    friend streamManager;
};

class DSTDLL branchEvent : public threadEvent{
public:
    branchEvent(std::function<bool()> aCondition, threadEvent* aTrueFunc, threadEvent* aFalseFunc) : threadEvent(QString()) {
        m_condition = aCondition;
        m_true = aTrueFunc;
        m_false = aFalseFunc;
    }
    void execute(std::shared_ptr<streamData> aStream) override {
        if (m_condition())
            m_true->execute(aStream);
        else
            m_false->execute(aStream);
    }
private:
    std::function<bool()> m_condition;
    threadEvent *m_true, *m_false;
};

class DSTDLL loopEvent : public threadEvent{
public:
    loopEvent(std::function<bool()> aCondition, threadEvent* aFunc) : threadEvent(QString()){
        m_condition = aCondition;
        m_th_func = aFunc;
    }
    void execute(std::shared_ptr<streamData> aStream) override {
        while (m_condition())
            m_th_func->execute(aStream);
    }
private:
    std::function<bool()> m_condition;
    threadEvent* m_th_func;
};

class DSTDLL streamManager{
public:
    //SINGLENTON(streamManager)
    static streamManager* instance();
public:
    QQmlApplicationEngine *engine = nullptr;
    friend threadEvent;
public:
    streamManager();
    ~streamManager();
    threadEvent* registerEvent(const QString& aSignal, const QString& aEventName, threadEvent* aEvent);
    threadEvent* registerEvent(const QString& aSignal, const QString& aEventName, streamFunc aFunc,
                                               const QString& aPreviouses = "", const QString& aNexts = "", int aThreadNo = 0);
    threadEvent* registerJSEvent(const QString& aSignal, const QString& aEventName, QJSValue aFunc,
                                                 const QString& aPreviouses = "", const QString& aNexts = "");
    void emitSignal(const QString& aSignal, std::shared_ptr<streamData> aInput = nullptr);
    void emitSignal(const QString& aSignal, const QString& aEvent, std::shared_ptr<streamData> aInput = nullptr);
    static void testSequence();
    static void testDynamicRegister();
public:
    void unregisterEvent(const QString& aSignal, const QString& aEventName);
private:
    void doRegisterEvent(const QString& aSignal, const QString& aEventName, threadEvent* aEvent);
    QThread* findThread(int aNo);
    QMap<QString, threadEvent*> m_events; // <signal, seqEvent>>
    QMap<QString, QSet<QString>> m_signals_map;
    QMap<int, std::shared_ptr<QThread>> m_threads; // <signal, <threadno, thread>>
};

class DSTDLL regPipe
{
public:
    regPipe(const char* aSignal, const char* aEventName, std::function<std::shared_ptr<streamData>(std::shared_ptr<streamData>)> aEvent, int aThreadNo){
        streamManager::instance()->registerEvent(aSignal, aEventName, aEvent, "", "", aThreadNo);
    }
    regPipe(const char* aSignal, const char* aEventName, std::function<std::shared_ptr<streamData>(std::shared_ptr<streamData>)> aEvent, int aThreadNo, const char* aPrevious){
        streamManager::instance()->registerEvent(aSignal, aEventName, aEvent, aPrevious, "", aThreadNo);
    }
};

#define TRIG(aSignal, aStream) \
    dst::streamManager::instance()->emitSignal(aSignal, aStream);

#define TRIG2(aSignal, aEvent, aStream) \
    dst::streamManager::instance()->emitSignal(aSignal, aEvent, aStream);

#define STMJSON \
    std::make_shared<dst::streamJson>

#define REGISTERPipe(aSignal, aEventName, aEvent, aThreadNo)  \
static dst::regPipe g_Register##aSignal##aEventName(#aSignal, #aEventName, aEvent, aThreadNo);

#define REGISTERPipe2(aSignal, aEventName, aEvent, aThreadNo, aPrevious)  \
static dst::regPipe g_Register2##aSignal(#aSignal, #aEventName, aEvent, aThreadNo, #aPrevious);

}

#endif
