import QtQuick 2.12
import QtQuick.Controls 2.12
import UIManager 1.0

Rectangle{
    property double actualminsize
    property string measurement_unit
    property int unit_pixel
    width: 100
    height: 40
    z: 2
    visible: false
    color: UIManager.layerTwoColor
    Row{
        leftPadding: parent.width * 0.1
        width: parent.width * 0.8
        height: parent.height

        Timer{
            property int step
            id: pressMonitor
            interval: 1
            repeat: true
            onTriggered: function(aParam){
                UIManager.setCommand({signal2: 'modifyMinSize', type: 'nullptr', param: {step: step}}, null)
                //console.log(aParam)
            }
        }

        CustomButton0{
            height: parent.height
            width: parent.width * 0.2
            text.text: "<"
            text.color: UIManager.fontColor
            color: "transparent"
            onClicked: UIManager.setCommand({signal2: "modifyMinSize", type: "nullptr", param: {step: - 1}}, null)
            btn.onPressAndHold: {
                pressMonitor.step = - 1
                pressMonitor.start()
            }
            btn.onReleased: {
                pressMonitor.stop()
            }
        }
        Text{
            height: parent.height
            width: parent.width * 0.6
            text: "10px"
            color: UIManager.fontColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            Component.onCompleted: {
                UIManager.registerPipe("modifyMinSize", "mdyGUI", function(aInput){
                    //text = aInput["minsize"] + "px"
                    actualminsize = aInput["minsize"]
                    text = actualminsize * unit_pixel + measurement_unit
                })
                UIManager.registerPipe("setMinSizeMode", "mdyGUI2", function(aInput){
                    //text = aInput["minsize"]
                    actualminsize = parseFloat(aInput["minsize"].substring(0, aInput["minsize"].indexOf("px")))
                    text = actualminsize * unit_pixel + measurement_unit
                })
                UIManager.registerPipe("updateLabelListGUI", "mdyGUI3", function(aInput){
                    UIManager.setCommand({signal2: "getProjectSetup", type: "json"}, function(aInput2){
                        measurement_unit = aInput2["measurement_unit"] || "px"
                        unit_pixel = aInput2["unit_pixel"] || 1
                        text = actualminsize * unit_pixel + measurement_unit
                    })

                }, "mdytsk");
            }
        }
        CustomButton0{
            height: parent.height
            width: parent.width * 0.2
            text.text: ">"
            text.color: UIManager.fontColor
            color: "transparent"
            onClicked: UIManager.setCommand({signal2: "modifyMinSize", type: "nullptr", param: {step: 1}}, null)
            btn.onPressAndHold: {
                pressMonitor.step = 1
                pressMonitor.start()
            }
            btn.onReleased: {
                pressMonitor.stop()
            }
        }
    }

    Component.onCompleted: {
        UIManager.registerPipe("showMinSizeEdit", "mdyGUI", function(aInput){
                visible = true  //this will affect the display of texts, reason is unknown
                x = aInput['x']
                y = aInput['y']
        })
        UIManager.registerPipe("hideMinSizeEdit", "mdyGUI", function(aInput){
                visible = false
        })
    }
}
