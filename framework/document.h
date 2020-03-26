#ifndef REAL_FRAMEWORK_DOCUMENT_H_
#define REAL_FRAMEWORK_DOCUMENT_H_

#include <map>
#include <memory>
#include "rxmanager.h"
#include "util.h"
#include <QJsonArray>

namespace dst {

class DSTDLL configObject : public QObject{
public:
    configObject(const QJsonObject& aConfig);
    virtual std::shared_ptr<QJsonObject> getConfig();
    virtual void setConfig(const QJsonObject& aConfig, bool aOverwrite = false);
    virtual void replaceConfig(std::shared_ptr<QJsonObject> aConfig);
    virtual void saveConfig() {}
    static void overWriteJsonObject(const QJsonObject& aSource, QJsonObject& aTarget);
    static void saveJsonFileConfig(const QString& aName, QJsonObject& aConfig);
    static void loadJsonFileConfig(const QString& aName, QJsonObject& aConfig);
    static QString generateObjectID();
private:
    std::shared_ptr<QJsonObject> config_;
};

class DSTDLL cacheObject : public configObject{
public:
    class additionalParameters{

    };
public:
    virtual void deserialize(){}
public:
    template <typename T>
    static std::shared_ptr<T> createObject(const QJsonObject& aConfig, std::shared_ptr<additionalParameters> aParam = nullptr){
        auto ret = std::make_shared<T>(aConfig, aParam);
        ret->deserialize();
        return ret;
    }
    cacheObject(const QJsonObject& aConfig) : configObject(aConfig){

    }
    void replaceConfig(std::shared_ptr<QJsonObject> aConfig){
        configObject::replaceConfig(aConfig);
        deserialize();
    }
};

class DSTDLL document{
public:
    SINGLENTON(document)
    ~document();
    void* findModel(std::string aName);
    void saveConfig();
public:
    const QString modifyDocumentInfo = "mdydoc";
    const QString modifyGUIInfo = "mdyGUI";
    const QString modifyStorageInfo = "mdystg";
    const QString modifyProjectsInfo = "mdyprjs";
    const QString modifyProjectInfo = "mdyprj";
    const QString modifyTaskInfo = "mdytsk";
    const QString modifyImageInfo = "mdyimg";
    const QString modifyClientInfo = "mdyclt";
    const QString modifyResultInfo = "mdyret";
    const QString modifyUserInfo = "mdyusr";
    const QString modifyCommandInfo = "mdycmd";
private:
    std::map<std::string, configObject*> m_models;
protected:
    document();
};

#define SETINFO(aName)  \
REGISTERPipe(set##aName, mdybak, [](std::shared_ptr<dst::streamData> aInput){ \
    QJsonObject* prm = reinterpret_cast<dst::streamJson*>(aInput.get())->getData(); \
    dst::configObject* cfg = reinterpret_cast<dst::configObject*>(dst::document::instance()->findModel(#aName)); \
    if (cfg == nullptr) \
        dst::configObject::saveJsonFileConfig(#aName, *prm); \
    else \
        cfg->setConfig(*prm); \
    aInput->callback(nullptr); \
    return aInput; \
}, 0);

#define GETINFO(aName)  \
REGISTERPipe(get##aName, mdybak, [](std::shared_ptr<dst::streamData> aInput){ \
    dst::configObject* cfg = reinterpret_cast<dst::configObject*>(dst::document::instance()->findModel(#aName)); \
    if (cfg == nullptr){ \
        QJsonObject ret; \
        dst::configObject::loadJsonFileConfig(#aName, ret); \
        aInput->callback(&ret); \
    } \
    else \
        aInput->callback(cfg->getConfig().get()); \
    return aInput; \
}, 0);

}

#endif
