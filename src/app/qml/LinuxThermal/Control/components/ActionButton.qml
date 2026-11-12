import QtQuick
import QtQuick.Controls.Basic

Button {
    id: root
    implicitHeight: Math.max(40, contentItem.implicitHeight + 20)
    leftPadding: 16
    rightPadding: 16
    topPadding: 10
    bottomPadding: 10
    hoverEnabled: true
    opacity: enabled ? 1 : 0.48
    HoverHandler { cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }
    background: Rectangle {
        radius: Theme.controlRadius
        color: root.highlighted ? Theme.accent : root.hovered || root.down ? Theme.panelHover : Theme.panel
        border.color: root.visualFocus ? Theme.accent : root.highlighted ? Theme.accent : Theme.borderStrong
        border.width: root.visualFocus ? 2 : 1
        Behavior on color { ColorAnimation { duration: Theme.animationFast } }
    }
    contentItem: Text {
        text: root.text
        font.pixelSize: Theme.captionSize
        font.weight: Font.Medium
        color: root.highlighted ? Theme.accentForeground : Theme.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        textFormat: Text.PlainText
    }
    scale: down ? 0.96 : hovered ? 1.025 : 1
    transform: Translate {
        y: root.hovered && root.enabled ? -2 : 0
        Behavior on y { NumberAnimation { duration: Theme.animationMedium; easing.type: Easing.OutCubic } }
    }
    Behavior on scale { NumberAnimation { duration: Theme.animationMedium; easing.type: Easing.OutBack } }
}
