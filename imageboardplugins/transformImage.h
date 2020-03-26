#ifndef REAL_IMAGEBOARDPLUGINS_TRANSFORMIMAGE_H_
#define REAL_IMAGEBOARDPLUGINS_TRANSFORMIMAGE_H_

#include "../framework/ImageBoard.h"

namespace dst {

class DSTDLL transformImageBoard : public ImageBoardPlugin{
public:
    ~transformImageBoard() override;
    void setParent(ImageBoard* aParent) override;
    void showImage(std::shared_ptr<imageObject> aImageModel) override;
    void updatePaintNode(QSGNode* aBackground) override;
protected:
    QTransform m_trans; //wcs->scs
private:
    void calcTransform();
    QPoint m_last_pos;
    const QString mdyTransformImageBoard = "mdyTransformImageBoard";
};

}

#endif
