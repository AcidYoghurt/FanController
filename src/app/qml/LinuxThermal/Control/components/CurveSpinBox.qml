import QtQuick
import QtQuick.Controls.Basic

SpinBox {
    id: root
    implicitWidth: 136
    implicitHeight: 42
    font.pixelSize: Theme.captionSize
    leftPadding: 34
    rightPadding: 34
    opacity: enabled ? 1 : 0.5
    background: Rectangle {
        radius: Theme.controlRadius
        color: Theme.inset
        border.color: root.activeFocus || root.contentItem.activeFocus ? Theme.accent : Theme.borderStrong
        border.width: root.activeFocus || root.contentItem.activeFocus ? 2 : 1
    }
    up.indicator: Rectangle {
        x: root.mirrored ? 0 : root.width - width
        width: 32
        height: root.height
        radius: Theme.controlRadius
        color: root.up.pressed || root.up.hovered ? Theme.panelHover : "transparent"
        Text { anchors.centerIn: parent; text: "+"; color: Theme.text; font.pixelSize: Theme.bodySize; textFormat: Text.PlainText }
    }
    down.indicator: Rectangle {
        x: root.mirrored ? root.width - width : 0
        width: 32
        height: root.height
        radius: Theme.controlRadius
        color: root.down.pressed || root.down.hovered ? Theme.panelHover : "transparent"
        Text { anchors.centerIn: parent; text: "−"; color: Theme.text; font.pixelSize: Theme.bodySize; textFormat: Text.PlainText }
    }
}
