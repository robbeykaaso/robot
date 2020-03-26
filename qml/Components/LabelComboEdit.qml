import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.5
import UIManager 1.0

LabelEdit{
    id: edit
//    LayoutMirroring.enabled: true
//    Rectangle{
//        height: parent.height
//        width: 10
//        color: "blue"
        MouseArea{
            anchors.fill: parent
            onClicked: {
                label_recommend.open()
            }
        }

        Menu{
            property var ctrls: []
            id: label_recommend
            y: parent.height
            width: 150

            onOpened: {
                UIManager.setCommand({signal2: "getUsedLabelList", type: "json", param: {type: "shape"}}, function(aInfo){
                    for (var c in ctrls)
                        removeItem(ctrls[c])
                    var lbls = aInfo["labels"]
                    for (var i in lbls){
                        var src = "import QtQuick 2.12;import QtQuick.Controls 2.5;import UIManager 1.0;"
                        src += "MenuItem{"
                        src += "height: 30;"
                        src += "text: '" + lbls[i]["label"] + "';"
                        src += "onClicked: {edit.setNewLabel('" + lbls[i]["label"] + "');"
                        //src += "UIManager.setCommand({signal2: 'defaultCommand', type: 'nullptr'}, null);"
                        src += "}}"
                        var ctrl = Qt.createQmlObject(src, label_recommend)
                        ctrls.push(ctrl)
                        addItem(ctrl)
                    }
                });
            }
        }
//    }

//    Component.onCompleted: {
//        children[0].enabled = false
//    }
}
