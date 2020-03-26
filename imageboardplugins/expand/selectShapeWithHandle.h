#ifndef REAL_IMAGEBOARDPLUGINS_SELECTSHAPEWITHHANDLE_H_
#define REAL_IMAGEBOARDPLUGINS_SELECTSHAPEWITHHANDLE_H_

#include "../selectShape.h"

namespace dst{

class DSTDLL selectShapeWithHandleImageBoard : public selectShapeImageBoard{
public:
    ~selectShapeWithHandleImageBoard() override {
        dst::streamManager::instance()->unregisterEvent("shapeTransformChanged", mdySelectShapeWithHandleImageBoard + m_parent->getName());
        dst::streamManager::instance()->unregisterEvent("setImageTransform", mdySelectShapeWithHandleImageBoard + m_parent->getName());
    }
    void setParent(ImageBoard* aParent) override;
public:
    void updatePaintNode(QSGNode* aBackground) override;
    void uninstall() override;
protected:
    void tryTransformShape(QJsonObject& aConfig) override;
    void setSelected(const QString& aSelected) override;
private:
    void showLabelEdit();
    void hideLabelEdit();
    shapeList m_handles;
    int m_x_radius = 10;
    int m_y_radius = 10;
    QTransform m_trans;
    const QString mdySelectShapeWithHandleImageBoard = "mdySelectShapeWithHandleImageBoard";
};

}

#endif
