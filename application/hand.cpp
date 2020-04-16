#include "../framework/rxmanager.h"
#include "../framework/document.h"
#include "../framework/imageModel.h"

class robotHand : public dst::configObject{
private:
    void mouseDrag(const QJsonArray& aOrigin, const QJsonArray& aDelta){
        QPoint org(aOrigin[0].toDouble(), aOrigin[1].toDouble()),
            del(aDelta[0].toDouble(), aDelta[1].toDouble());
        SetCursorPos(org.x(), org.y());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, (del.x() + org.x()) * 65535 / 1920, (del.y() + org.y()) * 65535 / 1080, 0, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

        dst::showDstLog("mouse drag: org: " + QString::number(org.x()) + " " + QString::number(org.y()) +
                        "; del: " + QString::number(del.x()) + " " + QString::number(del.y()));
    }
    void mouseClick(const QJsonArray& aOrigin){
        QPoint org(aOrigin[0].toDouble(), aOrigin[1].toDouble());
        SetCursorPos(org.x(), org.y());
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

        dst::showDstLog("mouse click: org: " + QString::number(org.x()) + " " + QString::number(org.y()));
    }
    void mouseMove(const QJsonArray& aOrigin){
        QPoint org(aOrigin[0].toDouble(), aOrigin[1].toDouble());
        SetCursorPos(org.x(), org.y());

        dst::showDstLog("mouse move: org: " + QString::number(org.x()) + " " + QString::number(org.y()));
    }
public:
    robotHand() : configObject(QJsonObject()){
        dst::streamManager::instance()->registerEvent("controlWorld", "mdyhand",  [this](std::shared_ptr<dst::streamData> aInput){
            auto cfg = reinterpret_cast<dst::streamJson*>(aInput.get())->getData();
            if (cfg->value("type") == "drag")
                mouseDrag(cfg->value("org").toArray(), cfg->value("del").toArray());
            else if (cfg->value("type") == "move")
                mouseMove(cfg->value("org").toArray());
            else if (cfg->value("type") == "click")
                mouseClick(cfg->value("org").toArray());
            return aInput;
        }, "", "", 1);
    }
};

REGISTERPipe(hand, mdybak, [](std::shared_ptr<dst::streamData> aInput){
    aInput->callback(new robotHand());
    return aInput;
}, 0);
