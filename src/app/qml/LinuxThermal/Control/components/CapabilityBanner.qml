import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    required property string backendState
    required property bool partialCapability
    required property bool connected

    readonly property bool shown: !connected || backendState !== "ready" || partialCapability
    visible: shown
    implicitHeight: shown ? content.implicitHeight + 26 : 0
    radius: 14
    color: Qt.alpha(!connected || backendState === "fatal-error" ? Theme.danger : Theme.warning,
                    Theme.dark ? 0.16 : 0.09)
    border.color: !connected || backendState === "fatal-error" ? Theme.danger : Theme.warning

    RowLayout {
        id: content
        anchors.fill: parent
        anchors.margins: 13
        spacing: 12
        Rectangle {
            implicitWidth: 28; implicitHeight: 28; radius: 14
            color: !root.connected || root.backendState === "fatal-error" ? Theme.danger : Theme.warning
            Text { anchors.centerIn: parent; text: "!"; color: "white"; font.weight: Font.Bold; textFormat: Text.PlainText }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3
            Text {
                text: !root.connected ? qsTr("守护进程未连接")
                      : root.backendState === "unsupported" ? qsTr("未检测到受支持的风扇控制接口")
                      : root.backendState === "degraded" || root.partialCapability ? qsTr("部分硬件仅支持遥测")
                      : qsTr("风扇控制后端发生错误")
                color: Theme.text; font.pixelSize: 13; font.weight: Font.DemiBold; textFormat: Text.PlainText
            }
            Text {
                Layout.fillWidth: true
                text: !root.connected ? qsTr("请检查 systemd 服务和 D-Bus。")
                      : root.backendState === "unsupported" ? qsTr("应用仍可显示可用传感器，但不会尝试未验证的 EC 或厂商私有写入。")
                      : qsTr("只有明确暴露安全控制接口的风扇会响应模式切换，其余设备保持原有固件控制。")
                color: Theme.muted; font.pixelSize: 11; wrapMode: Text.WordWrap; textFormat: Text.PlainText
            }
        }
    }
}
