import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: root
    objectName: "appearancePopup"
    width: 260
    padding: 20
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle { color: Theme.panel; radius: Theme.cardRadius; border.color: Theme.borderStrong }
    contentItem: ColumnLayout {
        spacing: 12
        Text { text: qsTr("外观"); color: Theme.text; font.pixelSize: Theme.bodySize; font.weight: Font.Medium; textFormat: Text.PlainText }
        SelectField {
            objectName: "themeSelector"
            Layout.fillWidth: true
            model: [qsTr("跟随系统"), qsTr("浅色"), qsTr("深色")]
            currentIndex: AppSettings.themeMode
            onActivated: AppSettings.themeMode = currentIndex
            Accessible.name: qsTr("配色主题")
        }
    }
    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.animationMedium }
            NumberAnimation { property: "scale"; from: 0.92; to: 1; duration: Theme.animationSlow; easing.type: Easing.OutBack }
        }
    }
    exit: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; to: 0; duration: Theme.animationFast }
            NumberAnimation { property: "scale"; to: 0.96; duration: Theme.animationFast; easing.type: Easing.InCubic }
        }
    }
}
