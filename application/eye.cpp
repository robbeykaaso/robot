#include "../framework/imageModel.h"
#include "../framework/rxmanager.h"
#include "../framework/document.h"
#include <QTimer>
#include <QScreen>
#include <QPixmap>

class robotEye : public dst::configObject{
    //Q_OBJECT
public:
    //int m_tick = 0;
    robotEye() : configObject(QJsonObject()){
        dst::streamManager::instance()->registerEvent("handleImage", "mdyeye",  [this](std::shared_ptr<dst::streamData> aInput){
            m_timer.start();
            return aInput;
        }, "mdybrain", "");

        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, [this](){
            QScreen *screen = QGuiApplication::primaryScreen();
            auto img = screen->grabWindow(0).toImage();
            //img.save(QString::number(m_tick++) + ".png");

            auto stm = std::make_shared<dst::imageObject::streamImage>(dst::Json("board", "panel"));
            auto doc = dst::cacheObject::createObject<dst::imageObject>(QJsonObject(), std::make_shared<dst::imageObject::imageAdditionalParameter>(img));
            stm->setImage(doc);
            TRIG("handleImage", stm);
        });
        m_timer.start(2000);
    }
private:
    QTimer m_timer;
};

REGISTERPipe(eye, mdybak, [](std::shared_ptr<dst::streamData> aInput){
    aInput->callback(new robotEye());
    return aInput;
}, 0);
