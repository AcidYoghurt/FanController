import QtQuick
import QtQuick.Layouts

Panel {
    id: root
    required property string title
    required property string value
    property string suffix: ""
    property string caption: ""
    property color accent: Theme.accent
    implicitWidth: 220

    ColumnLayout {
        width: parent.width
        spacing: 8
        RowLayout {
            Layout.fillWidth: true
            Rectangle { implicitWidth: 8; implicitHeight: 8; radius: 4; color: root.accent }
            Text { Layout.fillWidth: true; text: root.title; color: Theme.muted; font.pixelSize: 13; font.weight: Font.DemiBold; textFormat: Text.PlainText; elide: Text.ElideRight }
        }
        RowLayout {
            spacing: 6
            Text { text: root.value; color: Theme.text; font.pixelSize: 34; font.weight: Font.DemiBold; font.family: "monospace"; textFormat: Text.PlainText }
            Text { text: root.suffix; color: Theme.muted; font.pixelSize: 14; Layout.alignment: Qt.AlignBottom; Layout.bottomMargin: 5; textFormat: Text.PlainText }
        }
        Text { Layout.fillWidth: true; text: root.caption; color: Theme.subtle; font.pixelSize: 11; visible: text.length > 0; textFormat: Text.PlainText; elide: Text.ElideRight }
    }
}
