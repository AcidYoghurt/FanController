import QtQuick

QtObject {
    id: root
    required property var backend
    property int currentPage: 0
    property string displayedError: ""
    readonly property bool modeTransitioning: operationState.pendingMode.length > 0
    signal errorRaised

    // 先完成退场，再显示服务端确认的选中模式。
    readonly property QtObject operation: QtObject {
        id: operationState
        property string pendingMode: ""
        property bool replyFinished: false
        property bool succeeded: false
    }
    readonly property Timer exitDelay: Timer {
        interval: Theme.animationFast
        onTriggered: root.finishModeTransition()
    }
    readonly property Connections backendSignals: Connections {
        target: root.backend
        function onModeChanged() { root.finishModeTransition() }
        function onModeRequestFinished(success, mode) {
            if (!root.modeTransitioning || mode !== operationState.pendingMode)
                return
            operationState.replyFinished = true
            operationState.succeeded = success
            root.finishModeTransition()
        }
        function onLastErrorChanged() {
            if (root.backend.lastError.length > 0) {
                root.displayedError = root.backend.lastError
                root.errorRaised()
            }
        }
    }

    function requestMode(mode: string): void {
        if (!backend.canSetMode(mode))
            return
        operationState.pendingMode = mode
        operationState.replyFinished = false
        operationState.succeeded = false
        exitDelay.restart()
        backend.setMode(mode)
    }
    function finishModeTransition(): void {
        if (modeTransitioning && !exitDelay.running && operationState.replyFinished
                && (!operationState.succeeded || backend.mode === operationState.pendingMode))
            operationState.pendingMode = ""
    }
    function applyEditedCurve(): void {
        if (backend.connected && !backend.busy && backend.curveSupported)
            backend.applyCurve()
    }
    function dismissError(message: string): void {
        if (backend.lastError === message)
            backend.clearError()
        if (displayedError === message)
            displayedError = ""
    }
}
