import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Button {
    id: control
    required property string mode
    required property string title
    property string description: ""
    property string glyph: ""
    property bool selected: false
    property color accent: Theme.accent

    Accessible.name: title
    Accessible.description: description
    Accessible.role: Accessible.RadioButton
    Accessible.checked: control.selected
    implicitHeight: Math.max(112, modeContent.implicitHeight + 28)
    padding: 0
    hoverEnabled: true

    background: Rectangle {
        radius: 16
        color: control.selected ? Qt.alpha(control.accent, Theme.dark ? 0.20 : 0.11)
                                : control.hovered ? Theme.panelHover : Theme.panel
        border.color: control.selected ? control.accent : Theme.border
        border.width: control.selected ? 2 : 1
        Behavior on color { ColorAnimation { duration: Theme.animationFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.animationFast } }
    }

    contentItem: RowLayout {
        id: modeContent
        spacing: 15
        Rectangle {
            Layout.leftMargin: 18
            implicitWidth: 46; implicitHeight: 46; radius: 14
            color: Qt.alpha(control.accent, Theme.dark ? 0.22 : 0.14)
            Text { anchors.centerIn: parent; text: control.glyph; color: control.accent; font.pixelSize: 21; font.weight: Font.Bold; textFormat: Text.PlainText }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5
            Text { Layout.fillWidth: true; text: control.title; color: Theme.text; font.pixelSize: 15; font.weight: Font.DemiBold; textFormat: Text.PlainText; elide: Text.ElideRight }
            Text { Layout.fillWidth: true; text: control.description; color: Theme.muted; font.pixelSize: 12; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
        }
        Rectangle {
            Layout.rightMargin: 18
            implicitWidth: 20; implicitHeight: 20; radius: 10
            color: control.selected ? control.accent : "transparent"
            border.color: control.selected ? control.accent : Theme.borderStrong
            border.width: 2
            Text { anchors.centerIn: parent; text: "✓"; color: "white"; font.pixelSize: 12; font.weight: Font.Bold; visible: control.selected; textFormat: Text.PlainText }
        }
    }
}
