#include "../framework/rxmanager.h"
#include "../framework/document.h"
#include "../framework/imageModel.h"

class robotHand : public dst::configObject{
private:
    void mouseDrag(const QJsonArray& aOrigin, const QJsonArray& aDelta){
        SetCursorPos(aOrigin[0].toInt(), aOrigin[1].toInt());
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_MOVE, aDelta[0].toInt(), aDelta[1].toInt(), 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    }
public:
    robotHand() : configObject(QJsonObject()){
        dst::streamManager::instance()->registerEvent("controlWorld", "mdyhand",  [this](std::shared_ptr<dst::streamData> aInput){
            auto cfg = reinterpret_cast<dst::streamJson*>(aInput.get())->getData();
            if (cfg->value("type") == "drag")
                mouseDrag(cfg->value("org").toArray(), cfg->value("del").toArray());
            return aInput;
        }, "", "", 2);
    }
};

REGISTERPipe(hand, mdybak, [](std::shared_ptr<dst::streamData> aInput){
    aInput->callback(new robotHand());
    return aInput;
}, 0);
