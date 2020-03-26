import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import TextImageBoard 1.0
import UIManager 1.0
import "../Components"

ColumnLayout{
    Rectangle{
        Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true
        Layout.preferredHeight: parent.height * 0.05
        color: "gray"
        Row{
            anchors.fill: parent
            Button{
                text: qsTr("file")
                onClicked: {
                    filemenu.y = height
                    filemenu.open()
                }
                Menu{
                    id: filemenu
                    MenuItem{
                        text: qsTr("new")
                        onClicked: {
                            UIManager.setCommand({signal2: "newTask", type: "nullptr"}, null)
                        }
                    }
                    MenuItem{
                        text: qsTr("load")
                        onClicked: {
                            filedialog.selectExisting = true
                            filedialog.cmd = "loadTask"
                            filedialog.open()
                        }
                    }

                    MenuItem{
                        text: qsTr("save as")
                        onClicked: {
                            filedialog.selectExisting = false
                            filedialog.cmd = "saveTask"
                            filedialog.open()
                        }
                    }
                    FileDialog{
                        property string cmd: ""
                        id: filedialog
                        selectExisting: false
                        nameFilters: ["json文件 (*.json)"]
                        onAccepted: {
                            if (cmd != "")
                                UIManager.setCommand({signal2: cmd, type: "nullptr", param: {path: fileUrl.toString()}}, null)
                        }
                    }
                    MenuSeparator{

                    }
                    MenuItem{
                        text: qsTr("recent")
                    }
                }
            }
            Button{
                text: qsTr("view")
                onClicked: {
                    viewmenu.y = height
                    viewmenu.open()
                }
                Menu{
                    id: viewmenu
                    MenuItem{
                        text: qsTr("hideHello")
                        onClicked: {
                            if (text === qsTr("hideHello")){
                                UIManager.setCommand({signal2: "updateVisibleShape", type: "nullptr", param: {hide_label: ["hello"]}}, null)
                                text = qsTr("showHello")
                            }else{
                                UIManager.setCommand({signal2: "updateVisibleShape", type: "nullptr", param: {show_label: ["hello"]}}, null)
                                text = qsTr("hideHello")
                            }
                        }
                    }
                    MenuItem{
                        text: qsTr("hideLabel")
                        onClicked: {
                            UIManager.setCommand({signal2: "updateVisibleLabel", type: "nullptr"}, null)
                            if (text === qsTr("hideLabel")){
                                text = qsTr("showLabel")
                            }else{
                                text = qsTr("hidLabel")
                            }
                        }
                    }
                    MenuSeparator{

                    }
                    MenuItem{
                        text: qsTr("fitView")
                        onClicked: {
                            UIManager.setCommand({signal2: "zoomImage", type: "nullptr", param: {board: "panel0", step: 0}}, null)
                        }
                    }
                    MenuItem{
                        text: qsTr("zoomIn")
                        onClicked: {
                            UIManager.setCommand({signal2: "centerZoomImage", type: "nullptr", param: {board: "panel0", step: 1}}, null)
                        }
                    }

                    MenuItem{
                        text: qsTr("zoomOut")
                        onClicked: {
                            UIManager.setCommand({signal2: "centerZoomImage", type: "nullptr", param: {board: "panel0", step: - 1}}, null)
                        }
                    }

                    MenuSeparator{

                    }
                    MenuItem{
                        text: qsTr("addView")
                        onClicked: {
                            UIManager.setCommand({signal2: "addView", type: "nullptr"}, null)
                        }
                    }
                    MenuItem{
                        text: qsTr("deleteView")
                        onClicked: {
                            UIManager.setCommand({signal2: "deleteView", type: "nullptr"}, null)
                        }
                    }
                }
            }
            Button{
                text: qsTr("draw")
                onClicked: {
                    UIManager.setCommand({signal2: "installBoardMenuItem", type: "nullptr", param: {board: "panel0"}}, null)
                    drawmenu.y = height
                    drawmenu.open()
                }
                Menu{
                    id: drawmenu
                    MenuItem{
                        text: qsTr("Rectangle")
                        onClicked: {
                            UIManager.setCommand({signal2: "commandDrawRectangle", type: "nullptr"}, null)
                        }
                    }
                    MenuItem{
                        text: qsTr("Free")
                        onClicked: {
                            UIManager.setCommand({signal2: "commandDrawFree", type: "nullptr"}, null)
                        }
                    }
                }
            }
        }
    }
    Grid{
        property int viewcount: 1
        Layout.fillWidth: true
        Layout.preferredHeight: parent.height * 0.9
        columns: 1
        rows: 1
        LabelBoard{
            boardname: "panel0"
            width: parent.width / parent.columns
            height: parent.height / parent.rows
        }

        function updateRowsAndColumns(){
            var cnt = Math.ceil((Math.sqrt(viewcount)))
            columns = cnt
            rows = Math.ceil(viewcount / cnt)
            // console.log(viewcount + ";" + columns + ";" + rows)
        }

        function addEditView(){
            var brd = "panel" + (viewcount - 1)
            var src = "import QtQuick 2.12; import '../Components';"
            src += "LabelBoard{"
            src += "boardname: '" + brd + "';"
            src += "width: parent.width / parent.columns;"
            src += "height: parent.height / parent.rows;"
            src += "}"
            Qt.createQmlObject(src, this)
            UIManager.setCommand({signal2: "showSiblings", type: "nullptr", param: {board: brd}}, null)
            return brd;
        }

        function addView(){
            var brd = "panel" + (viewcount - 1)
            var src = "import QtQuick 2.12; import TextImageBoard 1.0;"
            src += "Rectangle{"
            src += "color: 'transparent';"
            src += "border.color: 'blue';"
            src += "border.width: 2;"
            src += "width: parent.width / parent.columns;"
            src += "height: parent.height / parent.rows;"
            src += "TextImageBoard{"
            src += "name: '" + brd + "';"
            src += "plugins: ['Transform'];"
            src += "siblings: ['result_panel'];"
            src += "width: parent.width;"
            src += "height: parent.height;"
            src += "z: -1;"
            src += "clip: true;"
            src += "}"
            src += "}"
            Qt.createQmlObject(src, this)
            UIManager.setCommand({signal2: "showSiblings", type: "nullptr", param: {board: brd}}, null)
            return brd;
        }

        function deleteView(){
            children[viewcount].destroy()
        }

        Component.onCompleted: {
            UIManager.registerPipe("addView", "mdyGUI", function(){
                viewcount++
                updateRowsAndColumns()
                return {newView: addView()}
            })
            UIManager.registerPipe("deleteView", "mdyGUI", function(){
                if (viewcount == 1)
                    return null
                viewcount--
                updateRowsAndColumns()
                deleteView()
            })
        }
    }

    Rectangle{
        Layout.alignment: Qt.AlignBottom
        Layout.fillWidth: true
        Layout.preferredHeight: parent.height * 0.05
        color: "gray"
    }

    /*TestBoard{
        Layout.fillHeight: true
        Layout.fillWidth: true
        //anchors.fill: parent
    }*/
}
