import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Button {
    id: control
    objectName: "mode-%1".arg(mode)
    required property string mode
    required property string title
    property string description: ""
    property bool presentationActive: true
    property bool selected: false
    property bool selectionVisible: selected
    property color accent: Theme.accent

    Accessible.name: title
    Accessible.description: description
    Accessible.role: Accessible.RadioButton
    Accessible.checked: selected
    implicitHeight: modeContent.implicitHeight + 36
    padding: 18
    hoverEnabled: true
    HoverHandler { cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }
    scale: down ? 0.975 : hovered ? 1.012 : 1
    transform: Translate {
        y: control.hovered && control.enabled ? -3 : 0
        Behavior on y { NumberAnimation { duration: Theme.animationMedium; easing.type: Easing.OutCubic } }
    }
    Behavior on scale { NumberAnimation { duration: Theme.animationMedium; easing.type: Easing.OutBack } }

    background: Rectangle {
        radius: Theme.controlRadius
        color: control.hovered ? Theme.panelHover : Theme.inset
        border.color: control.visualFocus ? Theme.accent : Theme.border
        border.width: control.visualFocus ? 2 : 1
        Behavior on color { ColorAnimation { duration: Theme.animationFast } }
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: Qt.alpha(control.accent, Theme.dark ? 0.10 : 0.06)
            border.color: control.accent
            border.width: 2
            opacity: control.selectionVisible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Theme.animationFast; easing.type: Easing.OutCubic } }
        }
    }
    contentItem: ColumnLayout {
        id: modeContent
        spacing: 12
        opacity: control.enabled || control.selected ? 1 : 0.5
        RowLayout {
            Layout.fillWidth: true
            ThermalIcon {
                kind: control.mode === "automatic" ? "shield" : control.mode === "custom-curve" ? "curve" : "fan"
                color: control.accent
                RotationAnimator on rotation {
                    from: 0; to: 360; duration: 1100; loops: Animation.Infinite
                    running: control.mode === "full-speed" && control.selected && control.visible && control.presentationActive
                             && control.Window.window !== null && control.Window.window.visible
                }
            }
            Item { Layout.fillWidth: true }
            Rectangle {
                implicitWidth: 20; implicitHeight: 20; radius: 10
                color: control.selectionVisible ? control.accent : "transparent"
                border.color: control.selectionVisible ? control.accent : Theme.borderStrong
                ThermalIcon {
                    anchors.centerIn: parent
                    width: 14; height: 14; kind: "check"; color: Theme.panel
                    opacity: control.selectionVisible ? 1 : 0
                    scale: control.selectionVisible ? 1 : 0.35
                    Behavior on opacity { NumberAnimation { duration: Theme.animationFast } }
                    Behavior on scale { NumberAnimation { duration: Theme.animationSlow; easing.type: Easing.OutBack } }
                }
            }
        }
        Text { Layout.fillWidth: true; text: control.title; color: Theme.text; font.pixelSize: Theme.bodySize; font.weight: Font.Medium; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
        Text { Layout.fillWidth: true; text: control.description; color: Theme.muted; font.pixelSize: Theme.captionSize; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
    }
}
