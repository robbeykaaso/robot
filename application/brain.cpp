#include "../framework/rxmanager.h"
#include "../framework/document.h"
#include "../framework/imageModel.h"
#include "../framework/ImageBoard.h"
#include <iostream>
#include <QScreen>
#include <QDir>

/*HHOOK g_hook;
bool goahead = false;
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam){
//    std::cout << "hello" << std::endl;
    if (wParam == 39)
        goahead = true;
    else
        goahead = false;
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}*/

class robotBrain : public dst::configObject{
private:
    QJsonObject calcOperation(){
        QJsonObject ret;
        ret.insert("type", "drag");
        HWND pWind = FindWindow("Notepad", NULL);
        if (pWind){
            RECT rect;
            GetWindowRect(pWind, &rect);
            POINT pt = {rect.right, rect.top}, pt2 = {rect.left, rect.bottom};
            ret.insert("org", dst::JArray(pt2.x + 115, pt.y + 25));
            if (m_tick++ % 2 == 0)
                ret.insert("del", dst::JArray(400, 0));
            else
                ret.insert("del", dst::JArray(- 400, 0));
        }
        return ret;
    }
    std::shared_ptr<dst::imageObject> m_screen;
    QTransform m_trans;
    double m_ratiox, m_ratioy;
public:
    robotBrain() : configObject(QJsonObject()){

        dst::streamManager::instance()->registerEvent("setImageTransform", "mdybrain",  [this](std::shared_ptr<dst::streamData> aInput){
            auto cfg = reinterpret_cast<dst::ImageBoard::streamTransform*>(aInput.get());
            m_trans = cfg->getTransform();
            return aInput;
        });

        dst::streamManager::instance()->registerEvent("boardMousePress", "ndybrain",  [this](std::shared_ptr<dst::streamData> aInput){
            auto cfg = reinterpret_cast<dst::imageObject::streamImage*>(aInput.get());
            POINT w_pt;
            GetCursorPos(&w_pt);
            auto test = w_pt;
            return aInput;
        });

        dst::streamManager::instance()->registerEvent("boardKeyPress", "mdybrain",  [this](std::shared_ptr<dst::streamData> aInput){
            auto cfg = reinterpret_cast<dst::imageObject::streamImage*>(aInput.get())->getData();
            if (cfg->value("key") == 16777236)
                m_go = true;
            else
                m_go = false;
            return aInput;
        });

        dst::streamManager::instance()->registerEvent("handleImage", "mdybrain",  [this](std::shared_ptr<dst::streamData> aInput){
            auto cfg = reinterpret_cast<dst::imageObject::streamImage*>(aInput.get());
            auto img = cfg->getImage()->getImage();
            //TRIG("showImage", aInput);
            if (m_go){
                TRIG("controlWorld", STMJSON(calcOperation()));
            }
            return aInput;
        }, "", "", 1);

        dst::streamManager::instance()->registerEvent("writeImageConfig", "mdybrain",  [this](std::shared_ptr<dst::streamData> aInput){
            aInput->callback(nullptr);
            return aInput;
        }, "", "", 1);

        dst::streamManager::instance()->registerEvent("commandCrop", "mdybrain", [this](std::shared_ptr<dst::streamData> aInput){
            QScreen *screen = QGuiApplication::primaryScreen();
            auto test = m_screen;
            auto img = screen->grabWindow(0).toImage();
            if (m_screen->getShapeList()->size() > 0){
                auto shp = m_screen->getShapeList()->begin().value();
                auto rect = shp->getActBound();
                auto pt1 = m_trans.map(QPoint(rect.left(), rect.top())), pt2 = m_trans.map(QPoint(rect.right(), rect.bottom()));
                QDir dir("config_");
                img.copy(pt1.x() * m_ratiox, pt1.y() * m_ratioy, (pt2.x() - pt1.x()) * m_ratiox, (pt2.y() - pt1.y()) * m_ratioy).save("config_/" + QString::number(dir.entryList().size()) + ".png");
            }
            return aInput;
        }, "mdyGUI", "mdyGUI2");

        QScreen *screen = QGuiApplication::primaryScreen();
        auto img0 = screen->grabWindow(0).toImage();
        auto sz = screen->size();
        m_ratiox = img0.width() / sz.width(), m_ratioy = img0.height() / sz.height();
        QImage img(sz.width(), sz.height(), QImage::Format_ARGB32);
        img.fill("transparent");
        auto cfg = dst::Json("shapes", dst::Json("crop", dst::Json("type", "rectangle", "points", dst::JArray(100, 100, 200, 200))));
        cfg.insert("current_shape", "crop");
        m_screen = dst::cacheObject::createObject<dst::imageObject>(cfg, std::make_shared<dst::imageObject::imageAdditionalParameter>(img));

        auto stm = std::make_shared<dst::imageObject::streamImage>(dst::Json("board", "panel"));
        stm->setImage(m_screen);
        TRIG("showImage", stm);

       // HWND pWind = FindWindow("Notepad", NULL);

       // HINSTANCE tmp = HINSTANCE(GetWindowLongA(pWind, 0));
        //auto tmp = GetModuleHandle("kernel32.dll");
        //g_hook = SetWindowsHook(WH_KEYBOARD, KeyboardProc);
    }
    ~robotBrain(){
       // UnhookWindowsHookEx(g_hook);
    }
private:
    int m_tick = 0;
    bool m_go = false;
};

REGISTERPipe(collectShapeCommand, mdycrop, [](std::shared_ptr<dst::streamData> aInput){
    QJsonObject items;
    items.insert("crop", "commandCrop");
    aInput->callback(&items);
    return aInput;
}, 0);

REGISTERPipe(collectShapeCommand, mdywindow, [](std::shared_ptr<dst::streamData> aInput){
    QJsonObject items;
    items.insert("window", "setWindowStyle");
    aInput->callback(&items);
    return aInput;
}, 0);

REGISTERPipe(brain, mdybak, [](std::shared_ptr<dst::streamData> aInput){
    aInput->callback(new robotBrain());
    return aInput;
}, 0);


REGISTERPipe(initializeBackend, inistg, [](std::shared_ptr<dst::streamData> aInput){
    dst::document::instance()->findModel("brain");
    dst::document::instance()->findModel("eye");
    dst::document::instance()->findModel("hand");
    return aInput;
}, 0);
