import QtQuick
import QtQuick.Layouts

Panel {
    id: root

    required property string secureBootState
    required property string secureBootRequirement
    required property bool secureBootKnown
    required property bool secureBootEnabled
    required property bool secureBootDisabled

    readonly property color stateColor: secureBootState === "disabled" ? Theme.success
                                         : secureBootState === "enabled" ? Theme.warning
                                         : secureBootState === "unsupported" ? Theme.cool
                                         : Theme.subtle
    readonly property color requirementColor:
        secureBootRequirement === "disable-or-enroll-key" ? Theme.danger
        : secureBootRequirement === "not-required" ? Theme.success
        : Theme.warning
    readonly property string stateText:
        !secureBootKnown ? qsTr("无法判断")
        : secureBootDisabled ? qsTr("已关闭")
        : secureBootEnabled ? qsTr("已开启")
        : secureBootState === "unsupported" ? qsTr("未使用 UEFI Secure Boot")
        : qsTr("无法判断")
    readonly property string requirementText:
        secureBootRequirement === "disable-or-enroll-key"
        ? qsTr("需要关闭或注册模块密钥")
        : secureBootRequirement === "not-required" ? qsTr("不需要关闭")
        : qsTr("需要进一步确认")
    readonly property string explanation:
        secureBootRequirement === "disable-or-enroll-key"
        ? qsTr("当前机型的专用 ACPI 后端需要加载 acpi_call。可以关闭安全启动，或为模块签名并通过 MOK 注册。")
        : secureBootState === "disabled"
          ? qsTr("安全启动已经关闭。当前系统无需进行额外设置。")
          : secureBootState === "enabled"
            ? qsTr("安全启动保持开启。当前检测到的标准内核后端可以正常工作，无需关闭。")
            : secureBootState === "unsupported"
              ? qsTr("当前系统没有以 UEFI Secure Boot 模式启动，因此不需要关闭安全启动。")
              : qsTr("无法读取 EFI SecureBoot 变量。请在 BIOS/UEFI 设置或使用 mokutil 手动确认。")

    ColumnLayout {
        width: parent.width
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                Text {
                    Layout.fillWidth: true
                    text: qsTr("安全启动")
                    color: Theme.text
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    textFormat: Text.PlainText
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("检查 Secure Boot 当前状态，以及控制后端是否要求关闭或处理模块签名。")
                    color: Theme.muted
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    textFormat: Text.PlainText
                }
            }

            Rectangle {
                implicitWidth: 10
                implicitHeight: 10
                radius: 5
                color: root.stateColor
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: root.width >= 700 ? 2 : 1
            columnSpacing: 14
            rowSpacing: 14

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 104
                radius: Theme.controlRadius
                color: Theme.panelHover
                border.color: Theme.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 7
                    Text {
                        text: qsTr("是否已经关闭安全启动")
                        color: Theme.muted
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        textFormat: Text.PlainText
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.stateText
                        color: root.stateColor
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                        wrapMode: Text.WordWrap
                        textFormat: Text.PlainText
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 104
                radius: Theme.controlRadius
                color: Theme.panelHover
                border.color: Theme.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 7
                    Text {
                        text: qsTr("是否需要关闭安全启动")
                        color: Theme.muted
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        textFormat: Text.PlainText
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.requirementText
                        color: root.requirementColor
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                        wrapMode: Text.WordWrap
                        textFormat: Text.PlainText
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: recommendationText.implicitHeight + 26
            radius: Theme.controlRadius
            color: Qt.alpha(root.requirementColor, Theme.dark ? 0.13 : 0.07)
            border.color: Qt.alpha(root.requirementColor, 0.45)

            Text {
                id: recommendationText
                anchors.fill: parent
                anchors.margins: 13
                text: root.explanation
                color: Theme.text
                font.pixelSize: 11
                wrapMode: Text.WordWrap
                textFormat: Text.PlainText
            }
        }
    }
}
