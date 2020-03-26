#ifndef REAL_FRAMEWORK_IMAGEPROVIDER_H_
#define REAL_FRAMEWORK_IMAGEPROVIDER_H_

#include <QQuickImageProvider>
#include <QQmlApplicationEngine>
#include <memory>
#include "rxmanager.h"
#include "util.h"

namespace dst {

class DSTDLL renderTrigger : public QObject
{
    Q_OBJECT
public:
    SINGLENTON(renderTrigger)
    explicit renderTrigger(QObject *aParent = nullptr) : QObject(aParent){}
    void trig(QJsonObject aParam = QJsonObject());
    void trig2(QJsonObject aParam = QJsonObject());
    void trigCommand(QJsonObject aParam = QJsonObject());
    void trigDrawResult(QJsonObject aParam = QJsonObject());
    void trigDrawImage(QJsonObject aParam = QJsonObject());
signals:
    void callQmlRefresh(QJsonObject aParam);
    void callQmlRefresh2(QJsonObject aParam);
    void callQmlSetCommand(QJsonObject aParam);
    void callQmlDrawResult(QJsonObject aParam);
    void callQmlDrawImage(QJsonObject aParam);
};

class DSTDLL deepImageProvider : public QQuickImageProvider
{
public:
    deepImageProvider(ImageType aType, Flags aFlags = Flags()) : QQuickImageProvider(aType, aFlags) {}
    void initialize(QQmlApplicationEngine* aEngine, const QString& aName);
    QImage requestImage(const QString& aId, QSize* aSize, const QSize& aRequestedSize);
    QImage getImage();
    static void showImage(const std::string& aName, const QImage& aImage);
private:
    void setImage(const std::string& aName, const QImage& aImage);
    void updateGUI(const std::string& aName);
    QImage m_image;
};

}

#endif
