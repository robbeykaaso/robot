#ifndef REAL_FRAMEWORK_COMMAND_H_
#define REAL_FRAMEWORK_COMMAND_H_

#include "rxmanager.h"
#include "ImageBoard.h"
#include "util.h"
#include <QMouseEvent>

namespace dst {

class commandManager;
class DSTDLL command{
public:
    virtual ~command(){}
    virtual void onMouseRelease(ImageBoard::streamMouse* aInput) {};
    virtual void onMousePress(ImageBoard::streamMouse* aInput) {}
    virtual void onMouseMove(ImageBoard::streamMouse* aInput) {}
    virtual void appendMenuItems(QJsonObject& aItems){}
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual void start() {}
    virtual bool stop() = 0;
    virtual QString commandName() = 0;
protected:
    friend commandManager;
};

class DSTDLL commandManager{
public:
    SINGLENTON(commandManager)
    void startCommand(std::shared_ptr<command> aCommand);
    void setDefaultCommand(const QJsonObject& aDefaultCommand) {m_default_command = aDefaultCommand;}
    void stopCommand(bool aGiveUpDefault = false);
    bool isBusy() {return m_command != nullptr;}
protected:
    commandManager();
private:
    std::shared_ptr<command> m_command;
    int m_current = - 1;
    std::vector<std::shared_ptr<command>> m_commands;
    QJsonObject m_default_command;
};

}

#endif
