import QtQuick 2.12
import QtQuick.Controls 2.5
import UIManager 1.0

Rectangle{
    property string board: ""
    z: 1
    visible: false
    width: 100
    height: 40
    Rectangle {
        width: parent.width
        height: parent.height
        color: "white"
        border.color: "blue"

        TextInput {
            property string selected: ""
            id: edit
            clip: true
            anchors.fill: parent
            anchors.margins: 4
            selectByMouse: true
            //focus: true
            font.pixelSize: parent.height - 10
            onTextEdited: {
                setNewLabel(text)
            }
        }
    }

    function setNewLabel(aLabel){
        if (edit.selected != "")
            //edit.text = aLabel
            UIManager.setCommand({signal2: "updateShapeLabel", type: "nullptr", param: {board: board, shape: edit.selected, label: aLabel}}, null)
    }

    Component.onCompleted: {
        UIManager.registerPipe("showLabelEdit", "mdyGUI" + board, function(aInput){
            if (aInput['board'] === board){
                visible = true  //this will affect the display of texts, reason is unknown
                x = aInput['x']
                y = aInput['y']
                edit.text = aInput['label']
                edit.selected = aInput['shape']
            }
        })
        UIManager.registerPipe("hideLabelEdit", "mdyGUI" + board, function(aInput){
            if (aInput['board'] === board){
                visible = false
                edit.text = ""
                edit.selected = ""
            }
        })
        UIManager.registerPipe("updateShapeLabel", "mdyGUI", function(aInput){
            if (edit.selected === aInput["shape"])
                edit.text = aInput["label"]
        })
    }
}
