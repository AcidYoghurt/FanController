import QtQuick
import QtQuick.Layouts

Panel {
    id: root

    required property string secureBootState
    required property bool secureBootKnown
    required property bool secureBootEnabled
    required property bool secureBootDisabled

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
                    text: qsTr("显示当前系统的 Secure Boot 状态。")
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
                    text: qsTr("安全启动状态")
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
    }
}
