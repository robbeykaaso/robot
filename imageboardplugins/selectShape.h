#ifndef REAL_IMAGEBOARDPLUGINS_SELECTSHAPE_H_
#define REAL_IMAGEBOARDPLUGINS_SELECTSHAPE_H_

#include "../framework/ImageBoard.h"

namespace dst {
    class DSTDLL selectShapeImageBoard : public ImageBoardPlugin{
    public:
        ~selectShapeImageBoard() override;
        void setParent(ImageBoard* aParent) override;
        void showImage(std::shared_ptr<imageObject> aImageModel) override;
        void updatePaintNode(QSGNode* aBackground) override;
    protected:
        virtual void tryTransformShape(QJsonObject& aConfig);
        virtual void setSelected(const QString& aSelected) {m_selected = aSelected;}
        virtual QColor getSelectColor() { return QColor("blue");}
        QString selectShape(const shapeList& aShapes, int aX, int aY);
        QString getSelected() {return m_selected;}
    private:
        QString m_selected = "";
        const QString mdySelectShapeImageBoard = "mdySelectShapeImageBoard";
    };
}

#endif
