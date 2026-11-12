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
        RowLayout {
            Layout.fillWidth: true
            Text { text: qsTr("运行模式"); color: Theme.text; font.pixelSize: Theme.headingSize; font.weight: Font.Medium; textFormat: Text.PlainText }
            Item { Layout.fillWidth: true }
            Text { visible: root.modeTransitioning; text: qsTr("正在切换…"); color: Theme.accent; font.pixelSize: Theme.captionSize; textFormat: Text.PlainText; Accessible.role: Accessible.StaticText }
        }
        Text { Layout.fillWidth: true; text: qsTr("模式会应用到所有具备对应控制能力的风扇区域；只读设备会明确显示为部分支持。"); color: Theme.muted; font.pixelSize: Theme.captionSize; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
        Item {
            id: modeArea
            Layout.fillWidth: true
            implicitHeight: modeGrid.implicitHeight
            readonly property ModeButton selectedButton: root.currentMode === "automatic" ? automaticButton
                                                 : root.currentMode === "full-speed" ? fullSpeedButton
                                                 : root.currentMode === "custom-curve" ? curveButton : null
            GridLayout {
                id: modeGrid
                width: parent.width
                columns: root.width >= 820 ? 3 : 1
                columnSpacing: 12
                rowSpacing: 12
                ModeButton {
                    id: automaticButton
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    presentationActive: root.inViewport
                    mode: "automatic"; title: qsTr("自动"); accent: Theme.success
                    description: qsTr("将可控风扇交还内核驱动、固件或设备控制器自动管理。")
                    selected: root.currentMode === mode
                    selectionVisible: selected && !root.modeTransitioning
                    enabled: root.automaticAvailable && !selected && !root.interactionBlocked
                    onClicked: root.modeRequested(mode)
                }
                ModeButton {
                    id: fullSpeedButton
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    presentationActive: root.inViewport
                    mode: "full-speed"; title: qsTr("全速散热"); accent: Theme.accent
                    description: qsTr("把所有可控 CPU、GPU 与系统风扇区域提升到 100%，适合快速降温和压力测试。")
                    selected: root.currentMode === mode
                    selectionVisible: selected && !root.modeTransitioning
                    enabled: root.fullSpeedAvailable && !selected && !root.interactionBlocked
                    onClicked: root.modeRequested(mode)
                }
                ModeButton {
                    id: curveButton
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    presentationActive: root.inViewport
                    mode: "custom-curve"; title: qsTr("自定义曲线"); accent: Theme.warning
                    description: qsTr("按每个控制区域绑定的温度传感器动态调速，并保留安全温度端点。")
                    selected: root.currentMode === mode
                    selectionVisible: selected && !root.modeTransitioning
                    enabled: root.customCurveAvailable && !selected && !root.interactionBlocked
                    onClicked: root.modeRequested(mode)
                }
            }
            Rectangle {
                objectName: "modeSelectionIndicator"
                x: modeArea.selectedButton ? modeArea.selectedButton.x + 18 : 18
                y: modeArea.selectedButton ? modeArea.selectedButton.y + modeArea.selectedButton.height - 5 : 0
                width: modeArea.selectedButton ? modeArea.selectedButton.width - 36 : 0
                height: 3
                radius: 1.5
                color: modeArea.selectedButton ? modeArea.selectedButton.accent : Theme.accent
                opacity: modeArea.selectedButton ? 1 : 0
                Behavior on x { NumberAnimation { duration: Theme.animationSlow; easing.type: Easing.OutCubic } }
                Behavior on y { NumberAnimation { duration: Theme.animationSlow; easing.type: Easing.OutCubic } }
                Behavior on color { ColorAnimation { duration: Theme.animationMedium } }
                Behavior on opacity { NumberAnimation { duration: Theme.animationMedium } }
            }
        }
    }
}
