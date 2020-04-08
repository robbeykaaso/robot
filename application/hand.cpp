#include "../framework/rxmanager.h"
#include "../framework/document.h"
#include "../framework/imageModel.h"

class robotHand : public dst::configObject{
private:
    void mouseDrag(const QJsonArray& aOrigin, const QJsonArray& aDelta){
        SetCursorPos(aOrigin[0].toDouble(), aOrigin[1].toDouble());
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_MOVE, aDelta[0].toDouble(), aDelta[1].toDouble(), 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    }
    void mouseClick(const QJsonArray& aOrigin){
        SetCursorPos(aOrigin[0].toDouble(), aOrigin[1].toDouble());
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    }
    void mouseMove(const QJsonArray& aOrigin){
        SetCursorPos(aOrigin[0].toDouble(), aOrigin[1].toDouble());
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
