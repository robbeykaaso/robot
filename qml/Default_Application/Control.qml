import QtQuick 2.12
import QtQuick.Layouts 1.12
import "../Components"
import UIManager 1.0

ColumnLayout{
    Item{
        Layout.fillHeight: true
        Layout.fillWidth: true
        LabelBoard{
            boardname: "panel"
            anchors.fill: parent
        }
    }
   /* MouseArea{
        anchors.fill: parent
        onClicked: {
            var shapes = {
                "1_shape": {type: "polyline", points: [200, 200, 500, 500, 400, 800], label: "poly"},
                "2_shape": {type: "circle", center: [600, 1700], radius: 500, label: "circle"},
                "3_shape": {type: "ellipse", center: [1600, 700], xradius: 300, yradius: 600, label: "ellipse"},
                "5_shape": {type: "mask", points: [200, 200, 600, 600, 200, 600, 600, 200], line_color: "white"}
               // "0_shape": {type: "rectangle", points: [200, 200, 600, 600], label: "rect"}
            }
            UIManager.setCommand({signal2: "testShowImage", type: "nullptr", param: {board: "panel", path: "D:/download/glass/panel.png", shapes: shapes}}, null)
        }
    }*/

    Component.onCompleted: {
       // UIManager.setCommand({signal2: "testShowImage", type: "nullptr", param: {board: "panel", path: "D:/download/glass/panel.png"}}, null)

       // shapes = {
      //      "0_shape": {type: "rectangle", points: [200, 200, 600, 600], label: "rect", line_color: "green", text_location: "bottom"}
      //  }
      //  UIManager.setCommand({signal2: "testShowImage", type: "nullptr", param: {board: "result_panel", path: "D:/download/glass/panel.png", shapes: shapes}}, null)
    }

}
