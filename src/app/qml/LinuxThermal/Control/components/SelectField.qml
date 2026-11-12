import QtQuick
import QtQuick.Controls.Basic

ComboBox {
    id: root
    implicitHeight: 42
    font.pixelSize: Theme.captionSize
    leftPadding: 12
    rightPadding: 32
    opacity: enabled ? 1 : 0.5
    background: Rectangle {
        radius: Theme.controlRadius
        color: root.hovered ? Theme.panelHover : Theme.inset
        border.color: root.visualFocus ? Theme.accent : Theme.borderStrong
        border.width: root.visualFocus ? 2 : 1
    }
    indicator: ThermalIcon {
        x: root.width - width - 10
        y: (root.height - height) / 2
        width: 16
        height: 16
        kind: "chevron"
        color: Theme.muted
    }
    HoverHandler { cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }
}
