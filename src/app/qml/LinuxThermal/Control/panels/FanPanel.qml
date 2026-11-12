pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

Panel {
    id: root
    required property var fanModel

    ColumnLayout {
        width: parent.width
        spacing: 14
        Text { text: qsTr("风扇与控制能力"); color: Theme.text; font.pixelSize: Theme.headingSize; font.weight: Font.DemiBold; textFormat: Text.PlainText }
        Text { Layout.fillWidth: true; text: qsTr("遥测可用并不代表可以控制。每个通道会单独标记能力。"); color: Theme.muted; font.pixelSize: Theme.captionSize; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
        Repeater {
            model: root.fanModel
            delegate: Rectangle {
                id: fanRow
                required property string displayName
                required property string deviceKind
                required property int rpm
                required property int duty
                required property bool rpmAvailable
                required property bool controllable
                required property bool zeroRpmActive
                required property string status
                Layout.fillWidth: true
                implicitHeight: rowContent.implicitHeight + 26
                radius: Theme.controlRadius
                color: Theme.panelHover
                border.color: Theme.border
                RowLayout {
                    id: rowContent
                    anchors.fill: parent
                    anchors.margins: 13
                    spacing: 12
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        Text { Layout.fillWidth: true; text: fanRow.displayName; color: Theme.text; font.pixelSize: Theme.captionSize; font.weight: Font.DemiBold; textFormat: Text.PlainText; elide: Text.ElideRight }
                        Text {
                            Layout.fillWidth: true
                            text: fanRow.controllable ? qsTr("可控制")
                                  : fanRow.status === "firmware-managed"
                                    ? qsTr("由固件管理，未公开安全写接口")
                                    : qsTr("只读遥测")
                            color: fanRow.controllable ? Theme.success : Theme.subtle
                            font.pixelSize: Theme.captionSize; textFormat: Text.PlainText
                        }
                    }
                    ColumnLayout {
                        spacing: 2
                        Text { Layout.alignment: Qt.AlignRight; text: fanRow.rpmAvailable ? Number(fanRow.rpm).toLocaleString(Qt.locale(), "f", 0) + " RPM" : qsTr("转速未知"); color: Theme.text; font.pixelSize: Theme.captionSize; font.family: "monospace"; textFormat: Text.PlainText }
                        Text { Layout.alignment: Qt.AlignRight; text: fanRow.duty >= 0 ? qsTr("%1% 占空比").arg(fanRow.duty) : fanRow.zeroRpmActive ? qsTr("零转速") : ""; color: Theme.muted; font.pixelSize: Theme.captionSize; textFormat: Text.PlainText }
                    }
                }
            }
        }
        Text { visible: root.fanModel.count === 0; Layout.fillWidth: true; text: qsTr("未检测到风扇转速或 PWM 通道。"); color: Theme.muted; font.pixelSize: Theme.captionSize; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
    }
}
