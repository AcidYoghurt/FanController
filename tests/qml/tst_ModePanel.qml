import QtQuick
import QtTest
import LinuxThermal.Control

Item {
    id: root
    width: 1120
    height: 900
    Component {
        id: panelComponent
        ModePanel {
            width: 1020
            currentMode: "automatic"
            automaticAvailable: true
            fullSpeedAvailable: true
            customCurveAvailable: true
            interactionBlocked: false
            modeTransitioning: false
        }
    }
    SignalSpy { id: requestSpy; signalName: "modeRequested" }
    TestCase {
        name: "ModeSelectionMotion"
        when: windowShown
        function cleanup() { requestSpy.target = null }
        function test_confirmedSwitch_data() {
            return [{tag: "horizontal", width: 1020, axis: "x"},
                    {tag: "vertical", width: 700, axis: "y"}]
        }
        function test_confirmedSwitch(data) {
            const panel = createTemporaryObject(panelComponent, root, {width: data.width})
            verify(!!panel, "Component exists")
            tryCompare(panel, "entranceOffset", 0)
            const marker = findChild(panel, "modeSelectionIndicator")
            verify(!!marker, "Object exists")
            const button = findChild(panel, "mode-full-speed")
            verify(!!button, "Object exists")
            const start = marker[data.axis]
            const destination = data.axis === "x" ? button.x + 18 : button.y + button.height - 5
            requestSpy.target = panel
            requestSpy.clear()
            mouseClick(button, button.width / 2, button.height / 2)
            tryCompare(requestSpy, "count", 1)
            tryCompare(panel, "currentMode", "automatic")
            panel.currentMode = "full-speed"
            tryVerify(() => marker[data.axis] > start && marker[data.axis] < destination)
            tryCompare(marker, data.axis, destination)
            panel.currentMode = "custom-curve"
            wait(60)
            panel.currentMode = "automatic"
            tryCompare(marker, data.axis, start)
        }
    }
}
