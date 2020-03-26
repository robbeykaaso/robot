import QtQuick 2.4
import QtQuick.Controls 2.5

CheckBox{
    property string labelColor: 'black'
    property string checkColor: "gray"
    property string backgroundColor: "white"
    property int customDiameter: 32
    width: 50
    font.pixelSize: customDiameter * 0.3
    //customize indicator:Rectangle

    indicator: Rectangle {
        id: chk
        implicitWidth: customDiameter * 0.4
        implicitHeight: customDiameter * 0.4
        x: parent.leftPadding
        y: parent.height / 2 - height / 2
        radius: width / 8
        color: backgroundColor
        border.color: parent.down ? checkColor : checkColor

        Rectangle {
            width: customDiameter * 0.2
            height: customDiameter * 0.2
            x: width * 0.5
            y: height * 0.5
            radius: x * 0.5
            color: parent.parent.down ? checkColor : checkColor
            visible: parent.parent.checked
        }
    }
    contentItem: Text {
        text: parent.text
        font: parent.font
        color: labelColor
        //y: parent.height / 2 - height / 2
        anchors.left: parent.left
        anchors.top: chk.top
        anchors.leftMargin: customDiameter
    } 
}
