import QtQuick
import QtQuick.Layouts

Item {
    id: root
    required property var points
    property real currentTemperature: -1
    property int currentDuty: -1
    readonly property bool hasCurrentReading: currentTemperature >= 0 && currentDuty >= 0

    property var displayedPoints: []
    property var previousPoints: []
    property var targetPoints: []
    property real morphProgress: 1
    property real revealProgress: 0
    property bool initialized: false
    property var viewport: null
    readonly property bool inViewport: {
        if (!visible || (Window.window && !Window.window.visible))
            return false
        if (!viewport)
            return true
        const layoutPosition = y + (parent ? parent.y : 0) - viewport.contentY
        if (!isFinite(layoutPosition))
            return false
        const top = mapToItem(viewport, 0, 0).y
        return top < viewport.height - Math.min(120, height / 4) && top + height > 0
    }
    property bool presentationActive: inViewport

    function updateCurve(): void {
        const next = (points || []).map(point => ({temperature: Number(point.temperature), duty: Number(point.duty)}))
        if (!initialized || !presentationActive || displayedPoints.length !== next.length) {
            curveMorph.stop()
            displayedPoints = next
            canvas.requestPaint()
            return
        }
        previousPoints = displayedPoints
        targetPoints = next
        curveMorph.restart()
    }
    Component.onCompleted: {
        updateCurve()
        initialized = true
        if (presentationActive)
            revealAnimation.restart()
    }
    onPresentationActiveChanged: {
        if (!initialized)
            return
        if (presentationActive) {
            revealAnimation.restart()
        } else {
            curveMorph.stop()
            revealAnimation.stop()
            revealProgress = 0
            updateCurve()
        }
    }
    onMorphProgressChanged: {
        if (previousPoints.length !== targetPoints.length)
            return
        displayedPoints = targetPoints.map((point, index) => ({
            temperature: previousPoints[index].temperature + (point.temperature - previousPoints[index].temperature) * morphProgress,
            duty: previousPoints[index].duty + (point.duty - previousPoints[index].duty) * morphProgress
        }))
        canvas.requestPaint()
    }
    onRevealProgressChanged: canvas.requestPaint()
    NumberAnimation {
        id: curveMorph
        target: root
        property: "morphProgress"
        from: 0; to: 1
        duration: Theme.animationSlow
        easing.type: Easing.OutCubic
    }
    NumberAnimation {
        id: revealAnimation
        target: root
        property: "revealProgress"
        from: 0; to: 1
        duration: Theme.animationReveal
        easing.type: Easing.OutCubic
    }

    implicitHeight: 312
    Accessible.role: Accessible.Graphic
    Accessible.name: qsTr("温度与风扇占空比曲线")
    Accessible.description: hasCurrentReading
                            ? qsTr("当前温度 %1 摄氏度，占空比 %2%。控制点可在旁边的输入框中编辑。")
                              .arg(Number(currentTemperature).toLocaleString(Qt.locale(), "f", 1)).arg(currentDuty)
                            : qsTr("当前读数不可用。控制点可在旁边的输入框中编辑。")

    Canvas {
        id: canvas
        anchors.fill: parent
        anchors.bottomMargin: 26
        antialiasing: true
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const context = getContext("2d")
            context.reset()
            const left = Math.ceil(Theme.captionSize * 3.8)
            const right = 18
            const top = 18
            const bottom = 34
            const width = Math.max(1, canvas.width - left - right)
            const height = Math.max(1, canvas.height - top - bottom)
            const mapX = temperature => left + (temperature - 30) / 70 * width
            const mapY = duty => top + (100 - duty) / 100 * height

            context.strokeStyle = Theme.border
            context.lineWidth = 1
            context.fillStyle = Theme.subtle
            context.font = Math.round(Theme.captionSize) + "px sans-serif"
            for (let duty = 0; duty <= 100; duty += 20) {
                const y = mapY(duty)
                context.beginPath(); context.moveTo(left, y); context.lineTo(left + width, y); context.stroke()
                context.fillText(duty + "%", 4, y + 4)
            }
            for (let temperature = 30; temperature <= 100; temperature += 10) {
                const x = mapX(temperature)
                context.beginPath(); context.moveTo(x, top); context.lineTo(x, top + height); context.stroke()
                if (temperature % 20 === 0) {
                    const label = temperature + "°"
                    const labelWidth = context.measureText(label).width
                    context.fillText(label, Math.min(canvas.width - labelWidth,
                                                     x - labelWidth / 2), top + height + 22)
                }
            }

            context.save()
            context.beginPath()
            context.rect(left - 6, top - 8, (width + 12) * root.revealProgress, height + 16)
            context.clip()
            if (root.displayedPoints.length > 0) {
                context.strokeStyle = Theme.accent
                context.lineWidth = 3
                context.beginPath()
                for (let index = 0; index < root.displayedPoints.length; ++index) {
                    const point = root.displayedPoints[index]
                    const x = mapX(point.temperature)
                    const y = mapY(point.duty)
                    if (index === 0) context.moveTo(x, y); else context.lineTo(x, y)
                }
                context.stroke()
                context.lineTo(mapX(root.displayedPoints[root.displayedPoints.length - 1].temperature), mapY(0))
                context.lineTo(mapX(root.displayedPoints[0].temperature), mapY(0))
                context.closePath()
                const area = context.createLinearGradient(0, top, 0, top + height)
                area.addColorStop(0, Qt.alpha(Theme.accent, 0.22))
                area.addColorStop(1, Qt.alpha(Theme.accent, 0.02))
                context.fillStyle = area
                context.fill()
                context.fillStyle = Theme.accent
                for (let index = 0; index < root.displayedPoints.length; ++index) {
                    const point = root.displayedPoints[index]
                    context.beginPath(); context.arc(mapX(point.temperature), mapY(point.duty), 5, 0, Math.PI * 2); context.fill()
                }
            }

            context.restore()
            if (root.hasCurrentReading) {
                context.fillStyle = Theme.cool
                context.beginPath()
                context.arc(mapX(Math.max(30, Math.min(100, root.currentTemperature))),
                            mapY(Math.max(0, Math.min(100, root.currentDuty))),
                            6, 0, Math.PI * 2)
                context.fill()
            }
        }
    }

    RowLayout {
        anchors.left: parent.left
        anchors.leftMargin: 46
        anchors.bottom: parent.bottom
        spacing: 8
        Rectangle { implicitWidth: 14; implicitHeight: 3; radius: 1; color: Theme.accent }
        Text { text: qsTr("目标曲线"); color: Theme.muted; font.pixelSize: Theme.captionSize; textFormat: Text.PlainText }
        Item { implicitWidth: 12 }
        Rectangle { implicitWidth: 8; implicitHeight: 8; radius: 4; color: root.hasCurrentReading ? Theme.cool : Theme.subtle }
        Text { text: root.hasCurrentReading ? qsTr("当前读数") : qsTr("当前读数不可用"); color: Theme.muted; font.pixelSize: Theme.captionSize; textFormat: Text.PlainText }
    }

    onPointsChanged: updateCurve()
    onCurrentTemperatureChanged: canvas.requestPaint()
    onCurrentDutyChanged: canvas.requestPaint()
    Connections {
        target: Theme
        function onDarkChanged() { canvas.requestPaint() }
    }

}
