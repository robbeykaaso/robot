import QtQuick 2.12

Item{
    property var siblings: []

    function collectEdits(aEdits, aTarget, aLen){
        var ret = aLen
        for (var i in aTarget.children){
            if (aTarget.children[i].caption !== undefined){
                ret = Math.max(ret, aTarget.children[i].caption.width)
                aEdits.push(aTarget.children[i])
            }
        }
        return ret
    }

    function align(){
        var len = 0
        var edits = []
        len = collectEdits(edits, parent, len)
        for (var i in siblings){
            len = collectEdits(edits, siblings[i].parent, len)
        }
        for (var j in edits)
            edits[j].caption.width = len
    }

    Component.onCompleted: {
        align()
    }
}
