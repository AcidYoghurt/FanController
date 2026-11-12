import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    required property string version
    required property int backendCount
    implicitHeight: 40
    color: Theme.panel
    border.color: Theme.border
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pageMargin
        anchors.rightMargin: Theme.pageMargin
        Text { text: qsTr("守护进程 %1").arg(root.version || "--"); color: Theme.subtle; font.pixelSize: Theme.captionSize; textFormat: Text.PlainText }
        Item { Layout.fillWidth: true }
        Text { text: qsTr("已检测 %1 个后端").arg(root.backendCount); color: Theme.subtle; font.pixelSize: Theme.captionSize; textFormat: Text.PlainText }
    }
}
