import QtQuick
import QtQuick.Controls.Basic

Dialog {
    id: root
    required property string requestedMode
    required property bool requestStillValid
    signal modeConfirmed(string mode)

    modal: true
    implicitHeight: 250
    width: Math.min(480, parent ? parent.width - 40 : 480)
    title: requestedMode === "full-speed" ? qsTr("启用全部可控风扇全速？") : qsTr("切换散热模式？")
    closePolicy: Popup.CloseOnEscape

    contentItem: Label {
        width: 420
        wrapMode: Text.WordWrap
        color: Theme.text
        textFormat: Text.PlainText
        text: root.requestedMode === "full-speed"
              ? qsTr("所有明确可控的风扇区域将立即提升到 100%。只读风扇不会被写入，噪音和功耗可能明显增加。")
              : root.requestedMode === "custom-curve"
                ? qsTr("将启用每个可控区域已保存的温度曲线。传感器故障或达到安全温度时，对应区域会强制全速。")
                : qsTr("所有支持自动模式的风扇区域将交还内核驱动、固件或设备控制器管理。")
    }

    footer: DialogButtonBox {
        Button { text: qsTr("确认"); enabled: root.requestStillValid; DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole }
        Button { text: qsTr("取消"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
        onAccepted: {
            if (root.requestStillValid)
                root.modeConfirmed(root.requestedMode)
            root.accept()
        }
        onRejected: root.reject()
    }
}
