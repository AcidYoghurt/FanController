import QtQuick
import QtQuick.Layouts

RowLayout {
    id: root
    required property string title
    required property string description
    property string label: ""
    spacing: 16
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 6
        Text { text: root.title; color: Theme.text; font.pixelSize: Theme.titleSize; font.weight: Font.Medium; textFormat: Text.PlainText }
        Text { text: root.description; Layout.fillWidth: true; color: Theme.muted; font.pixelSize: Theme.captionSize; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
    }
    Text { visible: text.length > 0 && root.width > 800; text: root.label; color: Theme.subtle; font.pixelSize: Theme.captionSize; textFormat: Text.PlainText }
}
