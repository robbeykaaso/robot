import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.5
import "../Components"
import UIManager 1.0

ColumnLayout{
    ListView{
        Layout.fillHeight: true
        Layout.fillWidth: true
        delegate: Text{
             text: msg
             font.pixelSize: UIManager.fontTitleSize
             color: UIManager.fontColor
             wrapMode: Text.WordWrap
             width: parent.width
         }
         ScrollBar.vertical: ScrollBar {
             onVisualPositionChanged: function(){
                // var start = Math.ceil(imagelistmodel.count * visualPosition),
               //  end = Math.ceil(imagelistmodel.count * (visualPosition + visualSize))
                 //console.log(start + ';' + end)
             }
         }
        model: ListModel{
            /*ListElement{
                msg: "hello"
            }
            ListElement{
                msg: "world"
            }*/
            Component.onCompleted: {
                UIManager.registerPipe("addLogRecord", "mdyGUI", function(aInput){
                    append({msg: aInput["log_msg"]})
                })
            }
        }
    }
}
