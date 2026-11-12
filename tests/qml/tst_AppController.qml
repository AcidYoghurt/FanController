import QtQuick
import QtTest
import LinuxThermal.Control

Item {
    id: root
    width: 800
    height: 600

    Component {
        id: backendComponent
        QtObject {
            property bool allowMode: true
            property bool connected: true
            property bool busy: false
            property bool curveSupported: true
            property string mode: "automatic"
            property string lastError: ""
            property int modeRequests: 0
            property int curveRequests: 0
            property int clearedErrors: 0
            signal modeRequestFinished(bool success, string mode)
            function canSetMode(value) { return allowMode && !busy && mode !== value }
            function setMode(value) { modeRequests++ }
            function applyCurve() { curveRequests++ }
            function clearError() { lastError = ""; clearedErrors++ }
        }
    }
    Component { id: controllerComponent; AppController {} }
    SignalSpy { id: errorSpy; signalName: "errorRaised" }

    TestCase {
        name: "AppController"
        when: windowShown
        property var backend
        property var controller

        function init() {
            backend = createTemporaryObject(backendComponent, root)
            verify(!!backend, "Component exists")
            controller = createTemporaryObject(controllerComponent, root, {backend: backend})
            verify(!!controller, "Component exists")
        }
        function cleanup() {
            errorSpy.target = null
        }
        function test_rejectUnavailableMode() {
            backend.allowMode = false
            controller.requestMode("full-speed")
            compare(backend.modeRequests, 0)
            compare(controller.modeTransitioning, false)
        }
        function test_waitForReplyAfterInventory() {
            controller.requestMode("full-speed")
            backend.mode = "full-speed"
            compare(controller.modeTransitioning, true)
            backend.modeRequestFinished(true, "full-speed")
            tryCompare(controller, "modeTransitioning", false)
            compare(backend.mode, "full-speed")
        }
        function test_waitForInventoryAfterReply() {
            controller.requestMode("full-speed")
            backend.modeRequestFinished(true, "full-speed")
            wait(Theme.animationFast + 30)
            compare(controller.modeTransitioning, true)
            backend.mode = "full-speed"
            tryCompare(controller, "modeTransitioning", false)
        }
        function test_failureAllowsRetry() {
            controller.requestMode("full-speed")
            backend.modeRequestFinished(false, "full-speed")
            tryCompare(controller, "modeTransitioning", false)
            compare(backend.mode, "automatic")
            controller.requestMode("custom-curve")
            compare(backend.modeRequests, 2)
            compare(controller.modeTransitioning, true)
        }
        function test_ignoreUnrelatedReply() {
            controller.requestMode("full-speed")
            backend.modeRequestFinished(false, "custom-curve")
            backend.mode = "full-speed"
            wait(Theme.animationFast + 30)
            compare(controller.modeTransitioning, true)
            backend.modeRequestFinished(true, "full-speed")
            tryCompare(controller, "modeTransitioning", false)
        }
        function test_errorRaisedAndAcknowledged() {
            errorSpy.target = controller
            errorSpy.clear()
            backend.lastError = "service-error"
            compare(errorSpy.count, 1)
            compare(controller.displayedError, "service-error")
            controller.dismissError("service-error")
            compare(backend.clearedErrors, 1)
            compare(controller.displayedError, "")
        }
        function test_preserveNewerBackendError() {
            backend.lastError = "newer-error"
            controller.dismissError("older-error")
            compare(backend.lastError, "newer-error")
            compare(controller.displayedError, "newer-error")
            compare(backend.clearedErrors, 0)
        }
        function test_applyRequiresAllCapabilities_data() {
            return [
                {tag: "disconnected", connected: false, busy: false, supported: true, expected: 0},
                {tag: "busy", connected: true, busy: true, supported: true, expected: 0},
                {tag: "unsupported", connected: true, busy: false, supported: false, expected: 0},
                {tag: "ready", connected: true, busy: false, supported: true, expected: 1}
            ]
        }
        function test_applyRequiresAllCapabilities(data) {
            backend.connected = data.connected
            backend.busy = data.busy
            backend.curveSupported = data.supported
            controller.applyEditedCurve()
            compare(backend.curveRequests, data.expected)
        }
    }
}
