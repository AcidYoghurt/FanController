import QtQuick
import QtQuick.Layouts

Panel {
    id: root

    required property string secureBootState
    required property bool secureBootKnown
    required property bool secureBootEnabled
    required property bool secureBootDisabled
    required property string mokState
    required property bool mokKnown
    required property bool mokEnrolled

    readonly property color stateColor: secureBootState === "disabled" ? Theme.success
                                         : secureBootState === "enabled" ? Theme.warning
                                         : secureBootState === "unsupported" ? Theme.cool
                                         : Theme.subtle
    readonly property string stateText:
        !secureBootKnown ? qsTr("无法判断")
        : secureBootDisabled ? qsTr("已关闭")
        : secureBootEnabled ? qsTr("已开启")
        : secureBootState === "unsupported" ? qsTr("未使用 UEFI Secure Boot")
        : qsTr("无法判断")
    readonly property color mokColor: mokEnrolled ? Theme.success
                                       : mokState === "pending" ? Theme.warning
                                       : Theme.subtle
    readonly property string mokText:
        !mokKnown ? qsTr("无法判断")
        : mokEnrolled ? qsTr("已注册")
        : mokState === "pending" ? qsTr("等待重启确认")
        : mokState === "not-generated" ? qsTr("尚未生成")
        : qsTr("未注册")

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
                    font.pixelSize: Theme.headingSize
                    font.weight: Font.DemiBold
                    textFormat: Text.PlainText
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("显示当前系统的 Secure Boot 状态。")
                    color: Theme.muted
                    font.pixelSize: Theme.captionSize
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

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: detailColumn.implicitHeight + 30
            radius: Theme.controlRadius
            color: Theme.panelHover
            border.color: Theme.border

            ColumnLayout {
                id: detailColumn
                anchors.fill: parent
                anchors.margins: 15
                spacing: 6
                Text {
                    text: qsTr("安全启动状态")
                    color: Theme.muted
                    font.pixelSize: Theme.captionSize
                    font.weight: Font.DemiBold
                    textFormat: Text.PlainText
                }
                Text {
                    Layout.fillWidth: true
                    text: root.stateText
                    color: root.stateColor
                    font.pixelSize: Theme.bodySize
                    font.weight: Font.DemiBold
                    wrapMode: Text.WordWrap
                    textFormat: Text.PlainText
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: Theme.border
                }

                Text {
                    text: qsTr("安全启动密钥")
                    color: Theme.muted
                    font.pixelSize: Theme.captionSize
                    font.weight: Font.DemiBold
                    textFormat: Text.PlainText
                }
                Text {
                    Layout.fillWidth: true
                    text: root.mokText
                    color: root.mokColor
                    font.pixelSize: Theme.bodySize
                    font.weight: Font.DemiBold
                    wrapMode: Text.WordWrap
                    textFormat: Text.PlainText
                }
            }
        }
    }
}
