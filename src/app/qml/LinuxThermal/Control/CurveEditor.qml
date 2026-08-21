pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Panel {
    id: root

    required property var zoneModel
    required property var curveModel
    required property string selectedZoneId
    required property string selectedZoneName
    required property int minimumDuty
    required property int safetyTemperature
    required property int maximumTemperature
    required property real currentTemperature
    required property int currentDuty
    required property bool curveSupported
    required property bool connected
    required property bool busy
    required property bool conflict

    signal zoneRequested(string zoneId)
    signal pointEdited(int row, int temperature, int duty)
    signal saveRequested
    signal applyRequested
    signal resetRequested
    signal discardRequested

    function syncZoneSelection() {
        zoneSelector.currentIndex = zoneSelector.indexOfValue(root.selectedZoneId)
    }

    ColumnLayout {
        width: parent.width
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 14
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                Text { text: qsTr("自定义风扇曲线"); color: Theme.text; font.pixelSize: 18; font.weight: Font.DemiBold; textFormat: Text.PlainText }
                Text { Layout.fillWidth: true; text: qsTr("每个控制区域拥有独立曲线与安全边界。"); color: Theme.muted; font.pixelSize: 12; textFormat: Text.PlainText; elide: Text.ElideRight }
            }
            ComboBox {
                id: zoneSelector
                Layout.preferredWidth: Math.min(320, root.width * 0.42)
                model: root.zoneModel
                textRole: "displayName"
                valueRole: "zoneId"
                enabled: root.connected && !root.busy && root.zoneModel.count > 0
                Component.onCompleted: currentIndex = indexOfValue(root.selectedZoneId)
                onActivated: root.zoneRequested(currentValue)
                Accessible.name: qsTr("风扇控制区域")
            }
        }

        Connections {
            target: root
            function onSelectedZoneIdChanged() {
                root.syncZoneSelection()
            }
        }

        Rectangle {
            visible: root.conflict
            Layout.fillWidth: true
            implicitHeight: conflictRow.implicitHeight + 22
            radius: Theme.controlRadius
            color: Qt.alpha(Theme.warning, Theme.dark ? 0.16 : 0.09)
            border.color: Theme.warning
            RowLayout {
                id: conflictRow
                anchors.fill: parent
                anchors.margins: 11
                Text { Layout.fillWidth: true; text: qsTr("服务端曲线已变化，本地未保存编辑暂未覆盖。"); color: Theme.text; font.pixelSize: 11; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
                Button { text: qsTr("放弃本地编辑"); onClicked: root.discardRequested() }
            }
        }

        Text {
            visible: !root.curveSupported
            Layout.fillWidth: true
            text: root.zoneModel.count === 0 ? qsTr("没有检测到可控制的风扇区域。") : qsTr("所选区域不支持基于温度的自定义曲线。")
            color: Theme.muted; font.pixelSize: 12; wrapMode: Text.WordWrap; textFormat: Text.PlainText
        }

        GridLayout {
            visible: root.curveSupported
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            columns: root.width >= 780 ? 2 : 1
            columnSpacing: 20
            rowSpacing: 16

            CurveChart {
                Layout.fillWidth: true
                Layout.preferredWidth: 560
                Layout.minimumWidth: 300
                points: root.curveModel.points
                currentTemperature: root.currentTemperature
                currentDuty: root.currentDuty
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 360
                Layout.minimumWidth: 280
                spacing: 9
                Repeater {
                    model: root.curveModel
                    delegate: RowLayout {
                        id: pointRow
                        required property int index
                        required property int temperature
                        required property int duty
                        required property bool fixed
                        readonly property var allPoints: root.curveModel.points
                        readonly property var previousPoint: index > 0 ? allPoints[index - 1] : null
                        readonly property var nextPoint: index + 1 < allPoints.length ? allPoints[index + 1] : null
                        Layout.fillWidth: true
                        spacing: 8
                        Text { Layout.preferredWidth: 34; text: qsTr("P%1").arg(pointRow.index + 1); color: Theme.text; font.pixelSize: 12; font.weight: Font.DemiBold; textFormat: Text.PlainText }
                        SpinBox {
                            id: temperatureBox
                            Layout.fillWidth: true
                            from: pointRow.previousPoint ? Number(pointRow.previousPoint.temperature) + 1 : 30
                            to: pointRow.nextPoint ? Number(pointRow.nextPoint.temperature) - 1 : root.maximumTemperature
                            value: pointRow.temperature
                            editable: true
                            live: true
                            enabled: root.connected && !pointRow.fixed && !root.busy
                            Accessible.name: qsTr("P%1 温度").arg(pointRow.index + 1)
                            Accessible.description: pointRow.fixed ? qsTr("最高温度端点，只读") : qsTr("风扇曲线温度控制点")
                            textFromValue: function(number, locale) { return Number(number).toLocaleString(locale, "f", 0) + " °C" }
                            valueFromText: function(text, locale) { return Number.fromLocaleString(locale, text.replace("°C", "").trim()) }
                            onValueModified: root.pointEdited(pointRow.index, value, dutyBox.value)
                        }
                        SpinBox {
                            id: dutyBox
                            Layout.fillWidth: true
                            from: pointRow.previousPoint ? Number(pointRow.previousPoint.duty) : root.minimumDuty
                            to: pointRow.nextPoint ? Number(pointRow.nextPoint.duty) : 100
                            value: pointRow.duty
                            editable: true
                            live: true
                            enabled: root.connected && !pointRow.fixed && !root.busy
                            Accessible.name: qsTr("P%1 风扇占空比").arg(pointRow.index + 1)
                            Accessible.description: pointRow.fixed ? qsTr("最高占空比端点，只读") : qsTr("风扇曲线占空比控制点")
                            textFromValue: function(number, locale) { return Number(number).toLocaleString(locale, "f", 0) + " %" }
                            valueFromText: function(text, locale) { return Number.fromLocaleString(locale, text.replace("%", "").trim()) }
                            onValueModified: root.pointEdited(pointRow.index, temperatureBox.value, value)
                        }
                    }
                }
            }
        }

        Rectangle {
            visible: root.curveSupported
            Layout.fillWidth: true
            implicitHeight: safetyText.implicitHeight + 24
            radius: Theme.controlRadius
            color: Qt.alpha(Theme.warning, Theme.dark ? 0.11 : 0.07)
            border.color: Qt.alpha(Theme.warning, 0.4)
            Text {
                id: safetyText
                anchors.fill: parent
                anchors.margins: 12
                text: qsTr("此区域最低占空比为 %1%；曲线终点固定为 %2°C / 100%；达到 %3°C 或传感器失效时会提前触发全速保护。降温需求持续 5 秒后，占空比每秒最多降低 2%。").arg(root.minimumDuty).arg(root.maximumTemperature).arg(root.safetyTemperature)
                color: Theme.muted; font.pixelSize: 11; wrapMode: Text.WordWrap; textFormat: Text.PlainText
            }
        }

        RowLayout {
            visible: root.curveSupported
            Layout.fillWidth: true
            spacing: 10
            Button { text: qsTr("恢复默认"); enabled: root.connected && !root.busy; onClicked: root.resetRequested() }
            Item { Layout.fillWidth: true }
            Text { visible: root.curveModel.dirty; text: qsTr("有未保存更改"); color: Theme.warning; font.pixelSize: 11; textFormat: Text.PlainText }
            Button { text: qsTr("仅保存"); enabled: root.connected && !root.busy && root.curveModel.dirty; onClicked: root.saveRequested() }
            Button { text: qsTr("保存并启用全部区域曲线"); enabled: root.connected && !root.busy; highlighted: true; onClicked: root.applyRequested() }
        }
    }
}
