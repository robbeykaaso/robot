import QtQuick 2.12

Row{
    property alias caption: caption
    property alias editbackground: edit
    property alias input: value

    spacing: 5
    Text {
        id: caption
        text: ""
        anchors.verticalCenter: parent.verticalCenter
    }

    Rectangle {
        id: edit
        color: "white"
        border.color: "gray"
        TextInput {
            id: value
            clip: true
            anchors.fill: parent
            anchors.margins: 4
            selectByMouse: true
            //focus: true
            font.pixelSize: edit.height - 10
        }
    }
}
