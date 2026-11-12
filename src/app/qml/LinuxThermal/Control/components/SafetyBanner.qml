import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    required property bool active
    required property string message
    visible: active
    implicitHeight: active ? content.implicitHeight + 26 : 0
    radius: 14
    color: Qt.alpha(Theme.danger, Theme.dark ? 0.18 : 0.10)
    border.color: Theme.danger
    RowLayout {
        id: content
        anchors.fill: parent
        anchors.margins: 13
        spacing: 12
        Rectangle { implicitWidth: 28; implicitHeight: 28; radius: 14; color: Theme.danger; Text { anchors.centerIn: parent; text: "!"; color: Theme.statusForeground; font.weight: Font.Bold; textFormat: Text.PlainText } }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3
            Text { text: qsTr("安全保护已触发"); color: Theme.danger; font.pixelSize: Theme.bodySize; font.weight: Font.DemiBold; textFormat: Text.PlainText }
            Text { Layout.fillWidth: true; text: root.message; color: Theme.text; font.pixelSize: Theme.captionSize; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
        }
    }
}
