pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

Panel {
    id: root
    required property var backendModel

    ColumnLayout {
        width: parent.width
        spacing: 14

        Text {
            text: qsTr("检测到的控制后端")
            color: Theme.text
            font.pixelSize: 18
            font.weight: Font.DemiBold
            textFormat: Text.PlainText
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("后端按硬件能力自动加载。只读表示可以识别或读取，但不会执行未经验证的写入。");
            color: Theme.muted
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            textFormat: Text.PlainText
        }

        Repeater {
            model: root.backendModel

            delegate: Rectangle {
                id: backendRow
                required property string displayName
                required property string driver
                required property string status
                required property string detail
                required property var capabilities

                Layout.fillWidth: true
                implicitHeight: backendContent.implicitHeight + 26
                radius: Theme.controlRadius
                color: Theme.panelHover
                border.color: Theme.border

                ColumnLayout {
                    id: backendContent
                    anchors.fill: parent
                    anchors.margins: 13
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                Layout.fillWidth: true
                                text: backendRow.displayName
                                color: Theme.text
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                                textFormat: Text.PlainText
                            }
                            Text {
                                Layout.fillWidth: true
                                text: backendRow.driver
                                color: Theme.subtle
                                font.pixelSize: 10
                                font.family: "monospace"
                                elide: Text.ElideRight
                                textFormat: Text.PlainText
                            }
                        }

                        Rectangle {
                            implicitWidth: stateText.implicitWidth + 18
                            implicitHeight: 26
                            radius: 13
                            color: Qt.alpha(backendRow.status === "ready" ? Theme.success
                                            : backendRow.status === "read-only" ? Theme.warning
                                            : Theme.subtle,
                                            Theme.dark ? 0.18 : 0.11)
                            Text {
                                id: stateText
                                anchors.centerIn: parent
                                text: backendRow.status === "ready" ? qsTr("可控制")
                                      : backendRow.status === "read-only" ? qsTr("只读")
                                      : qsTr("不可用")
                                color: backendRow.status === "ready" ? Theme.success
                                       : backendRow.status === "read-only" ? Theme.warning
                                       : Theme.subtle
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                textFormat: Text.PlainText
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: backendRow.detail
                        color: Theme.muted
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        textFormat: Text.PlainText
                    }

                    RowLayout {
                        spacing: 8
                        Text {
                            text: backendRow.capabilities.telemetry ? qsTr("遥测 ✓") : qsTr("遥测 —")
                            color: backendRow.capabilities.telemetry ? Theme.cool : Theme.subtle
                            font.pixelSize: 10
                            textFormat: Text.PlainText
                        }
                        Text {
                            text: backendRow.capabilities.automatic ? qsTr("自动 ✓") : qsTr("自动 —")
                            color: backendRow.capabilities.automatic ? Theme.success : Theme.subtle
                            font.pixelSize: 10
                            textFormat: Text.PlainText
                        }
                        Text {
                            text: backendRow.capabilities.fullSpeed ? qsTr("满速 ✓") : qsTr("满速 —")
                            color: backendRow.capabilities.fullSpeed ? Theme.accent : Theme.subtle
                            font.pixelSize: 10
                            textFormat: Text.PlainText
                        }
                        Text {
                            text: backendRow.capabilities.customCurve ? qsTr("曲线 ✓") : qsTr("曲线 —")
                            color: backendRow.capabilities.customCurve ? Theme.warning : Theme.subtle
                            font.pixelSize: 10
                            textFormat: Text.PlainText
                        }
                    }
                }
            }
        }
    }
}
