pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: root
    required property int currentIndex
    property bool navigationEnabled: true
    signal pageRequested(int index)
    implicitWidth: 204
    implicitHeight: 42
    radius: Theme.controlRadius
    color: Theme.inset
    border.color: Theme.border

    Rectangle {
        objectName: "navigationIndicator"
        x: 4 + root.currentIndex * (width + 4)
        y: 4
        width: (root.width - 12) / 2
        height: root.height - 8
        radius: 7
        color: Theme.panel
        border.color: Theme.border
        Behavior on x { NumberAnimation { duration: Theme.animationMedium; easing.type: Easing.OutCubic } }
    }

    component NavigationButton: Button {
        id: button
        required property int pageIndex
        readonly property bool selected: root.currentIndex === pageIndex
        Layout.fillWidth: true
        Layout.fillHeight: true
        padding: 0
        enabled: root.navigationEnabled
        hoverEnabled: true
        scale: down ? 0.95 : 1
        Behavior on scale { NumberAnimation { duration: Theme.animationFast; easing.type: Easing.OutCubic } }
        Accessible.role: Accessible.PageTab
        Accessible.selected: selected
        onClicked: { if (!selected) root.pageRequested(pageIndex) }
        background: Rectangle {
            radius: 7
            color: !button.selected && button.hovered ? Theme.panelHover : "transparent"
            border.color: button.visualFocus ? Theme.accent : Theme.border
            border.width: button.visualFocus ? 2 : 0
        }
        contentItem: Text {
            text: button.text
            color: button.selected ? Theme.text : Theme.muted
            Behavior on color { ColorAnimation { duration: Theme.animationMedium } }
            font.pixelSize: Theme.captionSize
            font.weight: button.selected ? Font.Medium : Font.Normal
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            textFormat: Text.PlainText
        }
        HoverHandler { cursorShape: button.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }
    }
    RowLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4
        NavigationButton {
            objectName: "controlNavigationButton"
            pageIndex: 0
            text: qsTr("散热控制")
            Accessible.name: qsTr("散热控制页面")
        }
        NavigationButton {
            objectName: "infoNavigationButton"
            pageIndex: 1
            text: qsTr("设备信息")
            Accessible.name: qsTr("设备信息页面")
        }
    }
}
