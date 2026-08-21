import QtQuick
import QtQuick.Layouts

Panel {
    id: root
    required property string currentMode
    required property bool automaticAvailable
    required property bool fullSpeedAvailable
    required property bool customCurveAvailable
    required property bool interactionBlocked
    required property bool modeTransitioning
    signal modeRequested(string mode)

    ColumnLayout {
        width: parent.width
        spacing: 14
        Text { text: qsTr("运行模式"); color: Theme.text; font.pixelSize: 18; font.weight: Font.DemiBold; textFormat: Text.PlainText }
        Text { Layout.fillWidth: true; text: qsTr("模式会应用到所有具备对应控制能力的风扇区域；只读设备会明确显示为部分支持。"); color: Theme.muted; font.pixelSize: 12; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
        ModeButton {
            Layout.fillWidth: true
            mode: "automatic"; title: qsTr("自动"); glyph: "A"; accent: Theme.success
            description: qsTr("将可控风扇交还内核驱动、固件或设备控制器自动管理。")
            selected: root.currentMode === mode
            selectionVisible: selected && !root.modeTransitioning
            enabled: root.automaticAvailable && !selected && !root.interactionBlocked
            onClicked: root.modeRequested(mode)
        }
        ModeButton {
            Layout.fillWidth: true
            mode: "full-speed"; title: qsTr("全部可控风扇全速"); glyph: "F"; accent: Theme.accent
            description: qsTr("把所有可控 CPU、GPU 与系统风扇区域提升到 100%，适合快速降温和压力测试。")
            selected: root.currentMode === mode
            selectionVisible: selected && !root.modeTransitioning
            enabled: root.fullSpeedAvailable && !selected && !root.interactionBlocked
            onClicked: root.modeRequested(mode)
        }
        ModeButton {
            Layout.fillWidth: true
            mode: "custom-curve"; title: qsTr("自定义曲线"); glyph: "≈"; accent: Theme.warning
            description: qsTr("按每个控制区域绑定的温度传感器动态调速，并保留安全温度端点。")
            selected: root.currentMode === mode
            selectionVisible: selected && !root.modeTransitioning
            enabled: root.customCurveAvailable && !selected && !root.interactionBlocked
            onClicked: root.modeRequested(mode)
        }
    }
}
