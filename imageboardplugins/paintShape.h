#ifndef REAL_IMAGEBOARDPLUGINS_PAINTSHAPE_H_
#define REAL_IMAGEBOARDPLUGINS_PAINTSHAPE_H_

#include "../framework/command.h"

namespace dst {

class DSTDLL drawCommand : public command{
public:
    drawCommand(const QJsonObject& aConfig, std::shared_ptr<imageObject> aImage) : command(){
        m_image_board = aConfig.value("board").toString();
        m_image = aImage;
    }

    void onMouseMove(dst::ImageBoard::streamMouse* aInput) override;
    void start() override;
    bool stop() override;
    void undo() override;
    void redo() override;
protected:
    virtual void updateShape(const QString& aSignal, const QJsonObject& aConfig = QJsonObject());
    int m_drawing_state;
protected:
    std::shared_ptr<imageObject> m_image;
    std::shared_ptr<shapeObject> m_shape = nullptr; //不直接基于文件，而基于缓存结构，以防出现效率问题
    QString m_image_board;
    QString m_shape_id;
    QString m_image_id;
};

}

#endif
