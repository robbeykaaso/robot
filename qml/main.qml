import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.5
import QtQuick.Controls.Material 2.3
import UIManager 1.0

ApplicationWindow {
    id: mainwindow
    visible: true
    width: 200//Screen.desktopAvailableWidth
    x: Screen.desktopAvailableWidth - width
    height: Screen.desktopAvailableHeight
    //visibility: Window.Maximized
    title: qsTr("deepinspectapp")
    flags: Qt.Window | Qt.WindowStaysOnTopHint | Qt.FramelessWindowHint
    opacity: 0.6
    background: Rectangle{
        anchors.fill: parent
        color: "transparent"
    }

    Component.onCompleted: {
        UIManager.registerPipe("setWindowStyle", "mdyGUI", function(aInput){
            if (mainwindow.visibility !== Window.Maximized){
                mainwindow.flags = Qt.Window | Qt.FramelessWindowHint | Qt.FramelessWindowHint
                mainwindow.visibility = Window.Maximized
            }else{
                mainwindow.flags = Qt.Window | Qt.WindowStaysOnTopHint | Qt.FramelessWindowHint//Qt.WindowTitleHint
                mainwindow.width = 200
                mainwindow.height = Screen.desktopAvailableHeight
                mainwindow.visibility = Window.Windowed
            }
        })
        UIManager.registerPipe("commandCrop", "mdyGUI", function(aInput){
            opacity = 0
            return aInput
        })
        UIManager.registerPipe("commandCrop", "mdyGUI2", function(aInput){
            opacity = 0.6
            return aInput
        })
    }

    Component.onDestruction: {
        UIManager.setCommand({signal2: 'finalizeBackend', type: 'nullptr'}, null)
    }

  /*  FontLoader{
        source: "file:SOURCEHANSANSSC-REGULAR.OTF"
        Component.onCompleted: {
            UIManager.fontFamily = name
        }
    }*/

    StackView{
        property bool go: false
        id: stackview
        anchors.fill: parent
        initialItem: "Default_Application/Control.qml"
        Component.onCompleted: {
            UIManager.setCommand({signal2: 'initializeBackend', type: 'nullptr'}, null)
            UIManager.registerPipe("boardKeyPress", "mdyGUI", function(aInput){
                if (aInput["go"] && !go){
                    go = true
                    push("Default_Application/Log.qml")
                }
            }, "mdybrain")
        }
        Keys.onPressed: {
            if (event.key !== 16777236 && go){
                go = false
                pop()
                UIManager.setCommand({"signal2": "cropMode", type: "nullptr"}, null)
            }
        }
    }
}
