import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Window 2.12
import UIManager 1.0

Window{
    id: root
    title: qsTr("Progress")
    flags: Qt.Dialog  & Qt.FramelessWindowHint
    modality: Qt.WindowModal
    width: Screen.desktopAvailableWidth * 0.16
    height: Screen.desktopAvailableHeight * 0.1
    color: UIManager.layerTwoColor

    Column{
        x: parent.width * 0.1
        y: parent.height * 0.2
        width: parent.width * 0.8
        height: parent.height * 0.6
        spacing: height * 0.1
        Text{
            id: title
            text: ""
            color: UIManager.fontColor
            font.pixelSize: parent.height * 0.3
            font.family: UIManager.fontFamily
        }

        ProgressBar{
            height: parent.height * 0.5
            width: parent.width
            from: 0
            to: 1
            value: 0

            function updateProgress(aInfo){
                value = aInfo["progress"]
                title.text = aInfo["title"] + " (" + Math.round(value * 100) + "%)"
                if (!root.visible){
                    root.height = Screen.desktopAvailableHeight * 0.1 //height will change after each close, reason is unknown
                    root.show()
                }
                if (value == 1)
                    root.close()
                return aInfo
            }

            Component.onCompleted: {
                UIManager.registerPipe("updateProgress", "mdyGUI", updateProgress)
            }
        }
    }

}
