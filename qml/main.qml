import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.5
import QtQuick.Controls.Material 2.3
import UIManager 1.0

ApplicationWindow {
    id: mainwindow
    visible: true
    width: 600//Screen.desktopAvailableWidth
    height: 300//Screen.desktopAvailableHeight
    //visibility: Window.Maximized
    title: qsTr("deepinspectapp")
    flags: Qt.Window //| Qt.FramelessWindowHint
    opacity: 0.6
    background: Rectangle{
        anchors.fill: parent
        color: "transparent"
    }

    Component.onCompleted: {
        UIManager.registerPipe("setWindowStyle", "mdyGUI", function(aInput){
            if (mainwindow.visibility !== Window.Maximized){
                mainwindow.flags = Qt.Window | Qt.FramelessWindowHint
                mainwindow.visibility = Window.Maximized
            }else{
                mainwindow.flags = Qt.Window | Qt.WindowTitleHint
                mainwindow.width = 600
                mainwindow.height = 300
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
        id: stackview
        anchors.fill: parent
        initialItem: "Default_Application/Control.qml"
        Component.onCompleted: {
            UIManager.setCommand({signal2: 'initializeBackend', type: 'nullptr'}, null)
        }
    }
}
