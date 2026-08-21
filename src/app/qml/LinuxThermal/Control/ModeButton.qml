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
    property bool selectionVisible: selected
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
        color: control.hovered ? Theme.panelHover : Theme.panel
        border.color: Theme.border
        border.width: 1
        Behavior on color { ColorAnimation { duration: Theme.animationFast } }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: Qt.alpha(control.accent, Theme.dark ? 0.20 : 0.11)
            border.color: control.accent
            border.width: 2
            opacity: control.selectionVisible ? 1 : 0

            Behavior on opacity {
                OpacityAnimator {
                    duration: control.selectionVisible
                              ? Theme.animationMedium : Theme.animationFast
                    easing.type: control.selectionVisible
                                 ? Easing.OutCubic : Easing.InCubic
                }
            }
        }
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
            color: "transparent"
            border.color: Theme.borderStrong
            border.width: 2

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: control.accent
                opacity: control.selectionVisible ? 1 : 0

                Behavior on opacity {
                    OpacityAnimator {
                        duration: control.selectionVisible
                                  ? Theme.animationMedium : Theme.animationFast
                        easing.type: control.selectionVisible
                                     ? Easing.OutCubic : Easing.InCubic
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                text: "✓"
                color: "white"
                font.pixelSize: 12
                font.weight: Font.Bold
                opacity: control.selectionVisible ? 1 : 0
                textFormat: Text.PlainText

                Behavior on opacity {
                    OpacityAnimator {
                        duration: control.selectionVisible
                                  ? Theme.animationMedium : Theme.animationFast
                        easing.type: control.selectionVisible
                                     ? Easing.OutCubic : Easing.InCubic
                    }
                }
            }
        }
    }
}
