import QtQuick 2.12
import TextImageBoard 1.0
import ImageBoard 1.0
import UIManager 1.0

Grid{
    property int viewcount: 1
    columns: 1
    rows: 1
    id: root

    Rectangle{
        width: parent.width / parent.columns
        height: parent.height / parent.rows
        color: "transparent"
        LabelBoard{
            property var defaultplugins: ["Transform", "SelectShapeWithHandle", "TransformShape", "Paint"]
            property var childmode
            id: panel0
            z: 0
            boardname: "panel0"
            anchors.fill: parent
            boardplugins: defaultplugins
            boardsiblings: ["result0", "back0"]

            function setChildMode(aMode, aState){
                if (!childmode)
                    childmode = {}
                childmode[aMode] = aState
            }

            function noChildMode(){
                if (!childmode)
                    childmode = {}
                for (var i in childmode){
                    if (childmode[i])
                        return false
                }
                return true
            }

           /* */
        }
        ImageBoard{
            name: "back0"
            plugins: ["CustomTransform"]
            anchors.fill: parent
            z: - 3
            clip: true
        }
        TextImageBoard{
            name: "result0"
            plugins: ["Transform", "Paint", "SelectShape"]
            anchors.fill: parent
            z: - 2
            clip: true
            Component.onDestruction: {
                beforeDestroy()
            }
        }
        ImageBoard{
            name: "roi_panel"
            plugins: ["Transform", "ROI", "TransformShape"]
            anchors.fill: parent
            siblings: ["panel0", "back0", "result0"]
            z: - 1
            clip: true
            Component.onCompleted: {
                UIManager.registerPipe("setROIMode", "mdyGUI", function(aInput){
                    z = aInput["z"]
                    if (z > 0){
                        if (panel0.noChildMode())
                            panel0.boardplugins = ["Transform"]
                        panel0.setChildMode("roi", true)
                        UIManager.setCommand({signal2: "setMinSizeMode", type: "nullptr", param: {mode: false}}, null)
                    }else{
                        panel0.setChildMode("roi", false)
                        if (panel0.noChildMode("roi", false))
                            panel0.boardplugins = panel0.defaultplugins
                    }
                    return aInput
                })
            }
        }
        ImageBoard{
            name: "minsize_panel"
            plugins: ["Transform", "MinSize", "TransformShape", "Paint"]
            anchors.fill: parent
            siblings: ["panel0", "back0", "result0"]
            z: - 1
            clip: true
            Component.onCompleted: {
                UIManager.registerPipe("setMinSizeMode", "mdyGUI", function(aInput){
                    z = aInput["z"]
                    if (z > 0){
                        if (panel0.noChildMode())
                            panel0.boardplugins = ["Transform"]
                        panel0.setChildMode("minsize", true)
                        UIManager.setCommand({signal2: "setROIMode", type: "nullptr", param: {mode: false}}, null)
                    }else{
                        panel0.setChildMode("minsize", false)
                        if (panel0.noChildMode())
                            panel0.boardplugins = panel0.defaultplugins
                    }
                    return aInput
                })
            }
        }
        MinSizeEdit{
        }
        Text{
            text: qsTr("")
            padding: 5
            font.pixelSize: UIManager.fontTitleSize * 3
            color: "green"
            Component.onCompleted: {
                UIManager.registerPipe("updateImagePredictGUI", "mdyGUI", function(aInput){
                    text = aInput["result"]
                })
            }
        }
    }

    function updateRowsAndColumns(){
        var cnt = Math.ceil((Math.sqrt(viewcount)))
        columns = cnt
        rows = Math.ceil(viewcount / cnt)
        // console.log(viewcount + ";" + columns + ";" + rows)
    }

    /*function addEditView(){
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
    }*/

    /*function addView(aIndex){
        var brd = "panel" + aIndex
        var src = "import QtQuick 2.12; import TextImageBoard 1.0;"
        src += "Rectangle{"
        src += "color: 'transparent';"
        src += "border.color: 'blue';"
        src += "border.width: 2;"
        src += "width: parent.width / parent.columns;"
        src += "height: parent.height / parent.rows;"
        src += "TextImageBoard{"
        src += "name: '" + brd + "';"
        src += "plugins: ['CustomTransform'];"
        src += "siblings: ['result_panel'];"
        src += "width: parent.width;"
        src += "height: parent.height;"
        src += "z: -1;"
        src += "clip: true;"
        src += "}"
        src += "}"
        //console.log('hi')
        Qt.createQmlObject(src, root)
        //console.log('hi2')
        UIManager.setCommand({signal2: "showSiblings", type: "nullptr", param: {board: brd}}, null)
        return brd;
    }*/
    function addView(aIndex){
        var src = "import QtQuick 2.12;"
        src += "AttachViewBoard{"
        src += "idx: " + aIndex + ";"
        src += "}"
        Qt.createQmlObject(src, root)
        //UIManager.setCommand({signal2: "showSiblings", type: "nullptr", param: {board: brd}}, null)
        return "panel" + aIndex;
    }

    function deleteView(aIndex){
        children[aIndex].destroy()
    }

    Component.onCompleted: {
        UIManager.registerPipe("setDisplayMode", "mdyGUI", function(aInput){
            var ret = []
            var sum = aInput["sum"]
            if (sum > 0){
                var del = sum - viewcount
                var i
                if (del > 0){
                    viewcount += del
                    updateRowsAndColumns()
                    for (i = 0; i < del; ++i){
                        ret.push(addView(viewcount - del + i))
                    }
                }else if (del < 0){
                    del = - del
                    viewcount -= del
                    for (i = 0; i < del; ++i)
                        deleteView(viewcount + del - i - 1)
                    updateRowsAndColumns()
                }
            }
            return {newViews: ret}
        })

        UIManager.registerPipe("addView", "mdyGUI", function(aInput){
            var sum = aInput["sum"]
            var del = sum - viewcount
            var ret = []
            if (del > 0){
                viewcount += del
                updateRowsAndColumns()
                for (var i = 0; i < del; ++i)
                    ret.push(addView(viewcount - del + i))
            }
            return {newViews: ret}
        })
        UIManager.registerPipe("deleteView", "mdyGUI", function(aInput){
            var del = aInput["del"]
            if (viewcount - del <= 1)
                del = viewcount - 1
            viewcount -= del
            for (var i = 0; i < del; ++i)
                deleteView(viewcount + del - i - 1)
            updateRowsAndColumns()
        })
    }
}
