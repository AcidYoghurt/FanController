import QtQuick

Item {
    id: root
    required property var points
    property real currentTemperature: -1
    property int currentDuty: -1

    implicitHeight: 286

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const context = getContext("2d")
            context.reset()
            const left = 44
            const right = 18
            const top = 18
            const bottom = 34
            const width = Math.max(1, canvas.width - left - right)
            const height = Math.max(1, canvas.height - top - bottom)
            const mapX = temperature => left + (temperature - 30) / 70 * width
            const mapY = duty => top + (100 - duty) / 80 * height

            context.strokeStyle = Theme.border
            context.lineWidth = 1
            context.fillStyle = Theme.subtle
            context.font = "11px sans-serif"
            for (let duty = 20; duty <= 100; duty += 20) {
                const y = mapY(duty)
                context.beginPath(); context.moveTo(left, y); context.lineTo(left + width, y); context.stroke()
                context.fillText(duty + "%", 4, y + 4)
            }
            for (let temperature = 30; temperature <= 100; temperature += 10) {
                const x = mapX(temperature)
                context.beginPath(); context.moveTo(x, top); context.lineTo(x, top + height); context.stroke()
                if (temperature % 20 === 0)
                    context.fillText(temperature + "°", x - 8, top + height + 22)
            }

            if (root.points.length > 0) {
                context.strokeStyle = Theme.accent
                context.lineWidth = 3
                context.beginPath()
                for (let index = 0; index < root.points.length; ++index) {
                    const point = root.points[index]
                    const x = mapX(point.temperature)
                    const y = mapY(point.duty)
                    if (index === 0) context.moveTo(x, y); else context.lineTo(x, y)
                }
                context.stroke()
                context.fillStyle = Theme.accent
                for (let index = 0; index < root.points.length; ++index) {
                    const point = root.points[index]
                    context.beginPath(); context.arc(mapX(point.temperature), mapY(point.duty), 5, 0, Math.PI * 2); context.fill()
                }
            }

            if (root.currentTemperature >= 0) {
                context.fillStyle = Theme.cool
                context.beginPath()
                context.arc(mapX(Math.max(30, Math.min(100, root.currentTemperature))),
                            mapY(Math.max(20, root.currentDuty >= 0 ? root.currentDuty : 100)),
                            6, 0, Math.PI * 2)
                context.fill()
            }
        }
    }

    onPointsChanged: canvas.requestPaint()
    onCurrentTemperatureChanged: canvas.requestPaint()
    onCurrentDutyChanged: canvas.requestPaint()
    Connections {
        target: Theme
        function onDarkChanged() { canvas.requestPaint() }
    }

}
