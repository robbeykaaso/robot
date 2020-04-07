#ifndef REAL_FRAMEWORK_BOARD_H_
#define REAL_FRAMEWORK_BOARD_H_

#include "rxmanager.h"
#include "imageModel.h"
#include "util.h"
#include <memory>
#include <QJsonObject>
#include <QJsonArray>
#include <QMouseEvent>
#include <QQuickItem>
#include <QTransform>
#include <QSGFlatColorMaterial>
#include <QSGImageNode>
#include <QSGSimpleTextureNode>
#include <QQueue>

//canvas
/*Canvas{
                        id: cvs
                        anchors.fill: parent
                        function drawPoly(aContext, aPoly){
                            // setup the stroke
                            aContext.lineWidth = 4
                            aContext.strokeStyle = "blue"
                            // begin a new path to draw
                            aContext.beginPath()
                            var j = 0;
                            aContext.moveTo(aPoly.location[j++], aPoly.location[j++]);
                            for (;j < aPoly.location.length;)
                                aContext.lineTo(aPoly.location[j++], aPoly.location[j++]);
                            // stroke using line width and stroke style
                            aContext.stroke()
                        }

                        function drawText(aContext, aText){
                            aContext.strokeStyle = "red";
                            aContext.font = " 26px sans-serif";

                            aContext.beginPath();
                            aContext.text(aText.content , aText.base[0], aText.base[1]);
                            aContext.stroke();
                        }
                    }
                     var ctx = cvs.getContext("2d")
                     ctx.clearRect(0, 0, width, height);
                     cvs.drawPoly(ctx, {location: [0, 0, width, height]})
                                 //drawText(ctx, {content: 'from QML', base: [100, 100]})
                     cvs.requestPaint()*/

//ref from: https://www.codetd.com/en/article/6858788
/*
#include <QQuickPaintedItem>
#include <QPainter>
  class ShapeInfo : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QJsonObject content READ getContent WRITE setContent)
    Q_PROPERTY(QColor color READ getColor WRITE setColor)
public:
    ShapeInfo(QQuickItem *parent = nullptr);
    QJsonObject getContent() const;
    void setContent(const QJsonObject& aContent);
    QColor getColor() const;
    void setColor(const QColor& aColor);

    void paint(QPainter *aPainter) override;
    Q_INVOKABLE bool contains(const QPointF& aPoint) const override;
    void mousePressEvent(QMouseEvent *event) override;
    void  mouseMoveEvent(QMouseEvent *event) override;
    void  mouseReleaseEvent(QMouseEvent *event) override;
private:
    QJsonObject m_content;
    QColor m_color;
};*/

/*
 qmlRegisterType<ShapeInfo>("ShapeItem", 1, 0, "ShapeItem");
 ShapeItem{
id: sh
        color: "red"
                anchors.fill: parent
                                   content: {
        "type": "rectangle",
                 "geometry": [100, 100, 300, 300]
    }
}*/

//https://codar.club/blogs/qt-scene-graph-line-drawing.html

namespace dst {

class ImageBoard;

class DSTDLL ImageBoardPlugin{
public:
    virtual ~ImageBoardPlugin(){}
public:
    virtual void setParent(ImageBoard* aParent) { m_parent = aParent; }
    virtual void showImage(std::shared_ptr<imageObject> aImageModel){
        m_image_model = aImageModel;
    }
    virtual void updatePaintNode(QSGNode* aBackground){}
    virtual void uninstall(){}
protected:
    void update();
    ImageBoard* m_parent;
    std::shared_ptr<imageObject> m_image_model = nullptr;
};

class DSTDLL ImageBoardFilters{
public:
    ImageBoardFilters(ImageBoard* aParent);
    virtual ~ImageBoardFilters();
    virtual void initializeFromSource(const QString& aSource);
    virtual bool canDisplay(std::shared_ptr<shapeObject> aShapeObject);
protected:
    const QString mdyImageBoardFilters = "mdyImageBoardFilters";
    ImageBoard* m_parent;
    QSet<QString> m_labels;
};

class DSTDLL dsImageBoardFilters : public ImageBoardFilters{
public:
    dsImageBoardFilters(ImageBoard* aParent);
    ~dsImageBoardFilters() override;
    void initializeFromSource(const QString& aSource) override;
    bool canDisplay(std::shared_ptr<shapeObject> aShapeObject) override;
private:
    bool m_forbid_show = false;
};

class DSTDLL ImageBoard : public QQuickItem{
    Q_OBJECT
    Q_PROPERTY(QString name WRITE setName READ getName)
    Q_PROPERTY(QJsonArray plugins WRITE installPlugins)
    Q_PROPERTY(QJsonArray siblings WRITE setSiblings)
    Q_PROPERTY(QString filter WRITE setFilter)
    Q_PROPERTY(QString custom WRITE setCustomProperies)
public:
    Q_INVOKABLE void beforeDestroy();
public:
    friend ImageBoardPlugin;
    friend dsImageBoardFilters;
    class streamMouse : public streamJson{
    public:
        streamMouse(const QJsonObject& aData, std::function<void(void*)> aCallback = nullptr) : streamJson(aData, aCallback) {}
        void setButtons(const Qt::MouseButtons& aButtons) {
            m_buttons = aButtons;
        }
        std::shared_ptr<streamData> clone() override{
            auto ret = std::make_shared<streamMouse>(m_data, m_callback);
            ret->setButtons(*getButtons());
            return ret;
        }
        Qt::MouseButtons* getButtons() {return &m_buttons;}
    private:
        Qt::MouseButtons m_buttons;
    };
    class streamTransform : public streamJson{
    public:
        streamTransform(const QJsonObject& aData, std::function<void(void*)> aCallback = nullptr) : streamJson(aData, aCallback) {}
        void setTransform(const QTransform& aTransform) {
            m_transform = aTransform;
        }
        std::shared_ptr<streamData> clone() override{
            auto ret = std::make_shared<streamTransform>(m_data, m_callback);
            ret->setTransform(getTransform());
            return ret;
        }
        QTransform getTransform() {return m_transform;}
    private:
        QTransform m_transform;
    };

public:
    QString getName() const;
    //ImageBoardPlugin* findPlugin(const QString& aName);
public:    
    explicit ImageBoard(QQuickItem *parent = nullptr);
    ~ImageBoard() override;
    void showImage(std::shared_ptr<imageObject> aImageModel);
    void refresh();
    virtual void addShape(std::shared_ptr<shapeObject> aShape);
    virtual void removeShape(std::shared_ptr<shapeObject> aShape);
protected:
    QSGNode *updatePaintNode(QSGNode*, UpdatePaintNodeData* nodedata) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    virtual void setName(const QString& aName);
    void setFilter(const QString& aFilter);
    virtual void setCustomProperies(const QString& aProperies);
    QSGNode* m_trans_node = nullptr;
    std::shared_ptr<imageObject> m_image_model = nullptr;
private:
    void doShowImage();
    void installPlugins(const QJsonArray& aPlugins);
    void uninstallPlugins(bool aUninstall = false);
    void setSiblings(const QJsonArray& aSiblings);
    void trigSiblings(std::shared_ptr<streamJson> aInput, const QString& aSignal);
    void initializeFilter();
    std::shared_ptr<streamData> createMouseStream(QMouseEvent *aEvent);
    QString m_name = "";
    bool m_needUpdate;
    std::shared_ptr<imageObject> m_last_image_model = nullptr; //for clear shapes
    QQueue<ImageBoardPlugin*> m_update_queue; //provide update strategy for plugins
    QMap<QString, ImageBoardPlugin*> m_plugins; //provide expanded plugin interfaces
    QJsonArray m_siblings; //provide overlap show stragety
    QString m_filter = ""; //copy filter data from source filter
    std::shared_ptr<ImageBoardFilters> m_filters = nullptr;
private:
    std::shared_ptr<imageObject> m_post_image_model = nullptr;
    bool m_showing = false;
};

}

#endif
