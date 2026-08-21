pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

GridLayout {
    id: root
    required property var sensorModel
    property real cpuControlTemperature: -1
    columns: width >= 900 ? 3 : width >= 560 ? 2 : 1
    columnSpacing: 14
    rowSpacing: 14

    Repeater {
        model: root.sensorModel
        delegate: MetricCard {
            required property string displayName
            required property string deviceKind
            required property real temperature
            required property bool available
            readonly property real shownTemperature:
                deviceKind === "cpu" && root.cpuControlTemperature >= 0
                ? root.cpuControlTemperature : temperature
            visible: deviceKind === "cpu" || deviceKind === "gpu"
            Layout.fillWidth: true
            title: deviceKind === "cpu" ? qsTr("CPU 温度") : qsTr("GPU 温度")
            value: available ? Number(shownTemperature).toLocaleString(Qt.locale(), "f", 1) : "--"
            suffix: "°C"
            caption: deviceKind === "cpu" && root.cpuControlTemperature >= 0
                     ? qsTr("%1 · 8 秒控制平均值").arg(displayName)
                     : displayName
            accent: !available ? Theme.subtle
                    : shownTemperature >= 85 ? Theme.danger
                    : shownTemperature >= 70 ? Theme.warning : Theme.cool
        }
    }

    MetricCard {
        visible: root.sensorModel.count === 0
        Layout.fillWidth: true
        title: qsTr("温度传感器")
        value: "--"
        caption: qsTr("未检测到可读取的温度传感器")
        accent: Theme.subtle
    }
}
