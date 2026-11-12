import QtQuick
import QtQuick.Controls.Basic

Dialog {
    id: root
    required property string errorMessage
    signal dismissed(string message)

    modal: true
    width: Math.min(500, parent ? parent.width - 40 : 500)
    height: Math.min(420, parent ? parent.height - 80 : 420)
    title: qsTr("操作失败")
    onClosed: dismissed(errorMessage)

    background: Rectangle { color: Theme.panel; radius: Theme.cardRadius; border.color: Theme.borderStrong }

    contentItem: ScrollView {
        id: errorScroll
        clip: true
        Label {
            width: errorScroll.availableWidth
            text: root.errorMessage
            color: Theme.text
            wrapMode: Text.WrapAnywhere
            textFormat: Text.PlainText
        }
    }

    footer: DialogButtonBox {
        ActionButton { text: qsTr("关闭"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
        onRejected: root.reject()
    }
}
