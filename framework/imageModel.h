#ifndef REAL_FRAMEWORK_MODEL_H_
#define REAL_FRAMEWORK_MODEL_H_

#include "document.h"
#include <QImage>
#include <QSGNode>
#include <QQuickWindow>
#include "util.h"

namespace dst {

using pointList = std::vector<QPoint>;
class imageObject;

class DSTDLL shapeObject : public cacheObject{
public:
    class shapeAdditionalParameter : public additionalParameters{
        friend shapeObject;
    public:
        shapeAdditionalParameter(imageObject* aImage) {m_image = aImage;}
    private:
        imageObject* m_image;
    };
public:
    shapeObject(const QJsonObject& aConfig, std::shared_ptr<additionalParameters> aParam) : cacheObject(aConfig){
        if (aParam != nullptr){
            m_parent_image = reinterpret_cast<shapeAdditionalParameter*>(aParam.get())->m_image;
        }else
            throw "create shapeObject error";
    }
    virtual QSGNode* createQSGNode(QQuickWindow* aWindow);
    QSGNode* getQSGNode() { return m_node;}
    void QSGNodeRemoved() { m_node = nullptr;}
    virtual void transform(const QJsonObject& aTransform);
    virtual void setColor(const QColor& aColor);
    virtual QRect getActBound() {return QRect(m_bound[0], m_bound[1], m_bound[2] - m_bound[0], m_bound[3] - m_bound[1]);}
protected:
    virtual void getBoundBox(double* aBoundBox, const pointList& aPoints, const QJsonObject& aTransform = QJsonObject());
    void getZoomParameter(const QJsonObject& aTransform, QPoint& aOrigin, double& aRatioX, double& aRatioY);
    virtual void transformQSGNode(const pointList& aPointList);
    double m_bound[4]; //leftbottomrighttop
    pointList m_points;
    QSGGeometryNode* m_node = nullptr;
    imageObject* m_parent_image;
};

class DSTDLL polylineObject : public shapeObject{
public:
    polylineObject(const QJsonObject& aConfig, std::shared_ptr<additionalParameters> aParam) : shapeObject(aConfig, aParam){}
    void transform(const QJsonObject& aTransform) override;
    void deserialize() override;
    static void calcBoundBox(double* aBoundBox, const pointList& aPoints);
    static QJsonObject configTemplate(const QString& aType, const QPoint& aStart);
};

class DSTDLL ellipseObject : public shapeObject{
public:
    ellipseObject(const QJsonObject& aConfig, std::shared_ptr<additionalParameters> aParam) : shapeObject(aConfig, aParam){}
    void transform(const QJsonObject& aTransform) override;
    void deserialize() override;
    static QJsonObject configTemplate(const QString& aType, const QPoint& aStart);
protected:
    void getBoundBox(double* aBoundBox, const pointList& aPoints, const QJsonObject& aTransform = QJsonObject()) override;
};

class DSTDLL maskObject : public shapeObject{
public:
    maskObject(const QJsonObject& aConfig, std::shared_ptr<additionalParameters> aParam) : shapeObject(aConfig, aParam){}
    void transform(const QJsonObject& aTransform) override;
    void setColor(const QColor& aColor) override;
    QRect getActBound() override;
    void deserialize() override;
    QSGNode* createQSGNode(QQuickWindow* aWindow) override;
    static QJsonObject configTemplate(const QString& aType, const QPoint& aStart);
protected:
    void transformQSGNode(const pointList& aPointList) override;
    void doTransformQSGNode(const pointList& aPointList, const QColor& aColor);
    QQuickWindow* m_window;
};

using shapeList = QMap<QString, std::shared_ptr<shapeObject>>;

class DSTDLL imageObject : public cacheObject{
public:
    class imageAdditionalParameter : public additionalParameters{
        friend imageObject;
    public:
        imageAdditionalParameter(const QImage& aImage) {m_image = aImage;}
    private:
        QImage m_image;
    };
public:
    class streamImage : public streamJson{
    public:
        streamImage(const QJsonObject& aData, std::function<void(void*)> aCallback = nullptr) : streamJson(aData, aCallback) {}
        void setImage(std::shared_ptr<imageObject> aImage) {
            m_image = aImage;
        }
        std::shared_ptr<streamData> clone() override{
            auto ret = std::make_shared<streamImage>(m_data, m_callback);
            ret->setImage(getImage());
            return ret;
        }
        std::shared_ptr<imageObject> getImage() {return m_image;}
    private:
        std::shared_ptr<imageObject> m_image;
    };
public:
    imageObject(const QJsonObject& aConfig, std::shared_ptr<additionalParameters> aParam = nullptr) : cacheObject(aConfig){
        if (aParam != nullptr){
            m_image = reinterpret_cast<imageAdditionalParameter*>(aParam.get())->m_image;
        }
       // m_image = aImage;
    }
    void deserialize() override;
    QImage getImage() {return m_image;}
    QTransform getTrans() {return m_trans;}
    shapeList* getShapeList(){return &m_shapes;}
    QString getCurrentShape() {return getConfig()->value("current_shape").toString();}
public:
    void Zoom(int aStep, const QPointF& aCenter, double aRatio = 0);
    void Move(const QPointF& aDistance);
    void setTrans(const QTransform& aTransform);
    void removeShape(const QString& aID);
    void SaveShape(const QString& aID, const QJsonObject& aShape);
    void setCurrentShape(const QString& aID);
protected:
    QImage m_image;
private:
    void SaveTransform();
    QTransform m_trans;
    shapeList m_shapes;
};

}

#endif
