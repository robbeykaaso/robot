import QtQuick 2.12
import ImageBoard 1.0
import TextImageBoard 1.0

Rectangle{
    property int idx
    color: 'transparent'
    border.color: 'blue'
    border.width: 2
    width: parent.width / parent.columns
    height: parent.height / parent.rows
    TextImageBoard{
        name: "panel" + idx
        plugins: ["Transform"]
        siblings: ["result" + idx, "back" + idx]
        width: parent.width
        height: parent.height
        z: 0
        clip: true
    }
    TextImageBoard{
        name: "result" + idx
        plugins: ["Transform", "Paint"]
        width: parent.width
        height: parent.height
        z: -1
        clip: true
        Component.onDestruction: {
            beforeDestroy()
        }
    }
    ImageBoard{
        name: "back" + idx
        plugins: ["CustomTransform"]
        width: parent.width
        height: parent.height
        z: -2
        clip: true
    }
}
