import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.5
import "../Components"
import UIManager 1.0

ColumnLayout{
    ListView{
        id: root
        Layout.fillHeight: true
        Layout.fillWidth: true
        delegate: Text{
             width: 240
             height: 30
             text: msg
             font.pixelSize: UIManager.fontTitleSize
             color: "red"
            // wrapMode: Text.WordWrap
             clip: true
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
                    root.currentIndex = count - 1
                })
            }
        }
    }
}
