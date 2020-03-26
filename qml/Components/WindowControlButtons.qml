import QtQuick 2.12
import QtQuick.Window 2.12
import UIManager 1.0

Row{
    height: parent.height
    width: height * 3
    CustomButton0{
        width: parent.height
        height: parent.height
        color: "#015bac"
        Image{
            anchors.fill: parent
            source: "qrc:qml/images/minimum.png"
        }
        onClicked: {
            mainwindow.visibility = Window.Minimized
        }
    }
    CustomButton0{
        width: parent.height
        height: parent.height
        color: "#015bac"
        Image{
            anchors.fill: parent
            source: "qrc:qml/images/maximum.png"
        }
        onClicked: {
            if (mainwindow.visibility !== Window.Maximized){
                mainwindow.flags = Qt.Window | Qt.FramelessWindowHint
                mainwindow.visibility = Window.Maximized
            }else{
                mainwindow.flags = Qt.Window | Qt.WindowTitleHint
                mainwindow.visibility = Window.Windowed
            }
        }
    }
    CustomButton0{
        width: parent.height
        height: parent.height
        color: "#015bac"
        radius: 0
        Image{
            anchors.fill: parent
            source: "qrc:qml/images/trash2.png"
        }

        onClicked: {
            //saveadvise.show()
            Qt.quit()
        }
    }
}
