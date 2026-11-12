import QtQuick
import QtQuick.Layouts

RowLayout {
    id: root
    required property int index
    required property int temperature
    required property int duty
    required property bool fixed
    required property var allPoints
    required property int minimumDuty
    required property int maximumTemperature
    required property bool editable
    signal pointEdited(int temperature, int duty)
    readonly property var previousPoint: index > 0 ? allPoints[index - 1] : null
    readonly property var nextPoint: index + 1 < allPoints.length ? allPoints[index + 1] : null
    spacing: 8

    Text { Layout.preferredWidth: 34; text: qsTr("P%1").arg(root.index + 1); color: Theme.text; font.pixelSize: Theme.captionSize; font.weight: Font.Medium; textFormat: Text.PlainText }
    CurveSpinBox {
        id: temperatureBox
        objectName: "curveTemperature%1".arg(root.index)
        Layout.fillWidth: true
        from: root.previousPoint ? Number(root.previousPoint.temperature) + 1 : 30
        to: root.nextPoint ? Number(root.nextPoint.temperature) - 1 : root.maximumTemperature
        value: root.temperature
        editable: true
        live: true
        enabled: root.editable && !root.fixed
        Accessible.name: qsTr("P%1 温度").arg(root.index + 1)
        Accessible.description: root.fixed ? qsTr("最高温度端点，只读") : qsTr("风扇曲线温度控制点")
        textFromValue: function(number, locale) { return Number(number).toLocaleString(locale, "f", 0) + " °C" }
        valueFromText: function(text, locale) { return Number.fromLocaleString(locale, text.replace("°C", "").trim()) }
        onValueModified: root.pointEdited(value, dutyBox.value)
    }
    CurveSpinBox {
        id: dutyBox
        objectName: "curveDuty%1".arg(root.index)
        Layout.fillWidth: true
        from: root.previousPoint ? Number(root.previousPoint.duty) : root.minimumDuty
        to: root.nextPoint ? Number(root.nextPoint.duty) : 100
        value: root.duty
        editable: true
        live: true
        enabled: root.editable && !root.fixed
        Accessible.name: qsTr("P%1 风扇占空比").arg(root.index + 1)
        Accessible.description: root.fixed ? qsTr("最高占空比端点，只读") : qsTr("风扇曲线占空比控制点")
        textFromValue: function(number, locale) { return Number(number).toLocaleString(locale, "f", 0) + " %" }
        valueFromText: function(text, locale) { return Number.fromLocaleString(locale, text.replace("%", "").trim()) }
        onValueModified: root.pointEdited(temperatureBox.value, value)
    }
}
