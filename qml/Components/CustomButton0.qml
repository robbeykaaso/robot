import QtQuick 2.12
import QtQuick.Controls 2.5

Rectangle {
    property alias text: t      //导出Text实例，方便外部直接修改
    property alias btn: area
    signal clicked();               //自定义点击信号
    id: ctrl
    width: 40
    height: 20
    color: "gray"
    radius: 0
    Text {
        id: t
        //默认坐标居中
        anchors.centerIn: parent
        //默认文字对齐方式为水平和垂直居中
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        //默认宽度为parent的宽度，这样字太长超出范围时自动显示省略号
        width: parent.width
    }

    MouseArea {
        id: area
        anchors.fill: parent;
        hoverEnabled: parent.enabled;
        cursorShape: Qt.PointingHandCursor  //悬浮或点击时的鼠标样式
        onClicked: {
            parent.clicked();  //点击时触发自定义点击信号
            anim.start();
        }
    }

    SequentialAnimation {
        id: anim
        property string origin_color: ctrl.color

        PropertyAnimation {
            target: ctrl
            property: "color"
            to: "transparent"
            duration: 150

        }
        PropertyAnimation {
            target: ctrl
            property: "color"
            to: anim.origin_color
            duration: 150

        }
    }
}
