import QtQuick 2.12
import QtQuick.Controls 2.5
import ImageBoard 1.0
import UIManager 1.0

Rectangle{
    property string boardname: ""
    property var boardplugins: ["Image", "Transform", "SelectShapeWithHandle", "TransformShape", "Paint"]
    property var boardsiblings: []
    color: "transparent"
    border.color: "blue"
    border.width: 2
    ImageBoard{
        id: board
        name: boardname
        plugins: boardplugins
        siblings: boardsiblings
        width: parent.width
        height: parent.height
        //smooth: true
        //Layout.fillHeight: true
        //Layout.fillWidth: true
        z: -1
        clip: true
        Menu{
            property var dynamicItems: []
            id: boardmenu
            x: 600
            y: 400
            /*MenuItem{
                text: qsTr("Cancel")
                onClicked: {
                    UIManager.setCommand({signal2: 'defaultCommand', type: 'nullptr'}, null)
                }
            }*/
            Component.onCompleted: {
                UIManager.registerPipe("popupBoardMenu", "mdyGUI" + parent.name, function(aInput){
                    if (aInput['board'] === parent.name){
                        //console.log("hello")
                        var i
                        for (i in dynamicItems)
                            removeItem(dynamicItems[i])
                        dynamicItems = []
                        for (i in aInput['items']){
                            var src = "import QtQuick 2.12; import QtQuick.Controls 2.5;import UIManager 1.0;"
                            src += "MenuItem{"
                            src += "text: qsTr('" + i + "');"
                            src += "onClicked: {"
                            src += "UIManager.setCommand({signal2: '" + aInput['items'][i] + "', type: 'nullptr'}, null)"
                            src += "}"
                            src += "}"
                            var itm = Qt.createQmlObject(src, boardmenu)
                            dynamicItems.push(itm)
                            insertItem(i, itm)
                        }

                        if (dynamicItems.length > 0)
                            popup(aInput['x'], aInput['y'])
                    }
                }, "mdycmd")
            }
        }
    }

    /*LabelComboEdit{
        board: boardname
    }*/
}
