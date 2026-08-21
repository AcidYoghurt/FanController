import QtQuick
import QtQuick.Controls.Basic
import LinuxThermal.Control

ApplicationWindow {
    id: window
    width: 1120
    height: 780
    minimumWidth: 760
    minimumHeight: 620
    visible: true
    title: qsTr("Linux Thermal Control")
    color: Theme.window

    property string displayedError: ""
    property int currentPage: 0
    property bool modeTransitioning: false
    property string pendingMode: ""
    property bool modeReplyFinished: false
    property bool modeRequestSucceeded: false

    function finishModeTransitionIfReady() {
        if (modeTransitioning && !modeExitDelay.running && modeReplyFinished
                && (!modeRequestSucceeded || FanBackend.mode === pendingMode)) {
            modeTransitioning = false
            pendingMode = ""
        }
    }

    Timer {
        id: modeExitDelay
        interval: Theme.animationFast
        onTriggered: window.finishModeTransitionIfReady()
    }

    header: AppHeader {
        connected: FanBackend.connected
        busy: FanBackend.busy && !window.modeTransitioning
        refreshing: FanBackend.refreshing
        backendState: FanBackend.state
        systemName: FanBackend.systemName
        currentPage: window.currentPage
        navigationEnabled: !dashboard.busy
        onPageRequested: page => window.currentPage = page
        onRefreshRequested: FanBackend.refresh()
    }

    Dashboard {
        id: dashboard
        anchors.fill: parent
        backend: FanBackend
        currentPage: window.currentPage
        modeTransitioning: window.modeTransitioning
        onModeRequested: mode => {
            if (FanBackend.canSetMode(mode)) {
                window.pendingMode = mode
                window.modeReplyFinished = false
                window.modeRequestSucceeded = false
                window.modeTransitioning = true
                modeExitDelay.restart()
                FanBackend.setMode(mode)
            }
        }
        onEditedCurveApplyRequested: {
            if (FanBackend.connected && !FanBackend.busy && FanBackend.curveSupported)
                FanBackend.applyCurve()
        }
    }

    footer: AppFooter {
        version: FanBackend.daemonVersion
        backendCount: FanBackend.backends.count
    }

    Shortcut {
        sequences: [StandardKey.Refresh]
        enabled: !FanBackend.busy
        onActivated: FanBackend.refresh()
    }

    OperationErrorDialog {
        id: operationError
        anchors.centerIn: parent
        errorMessage: window.displayedError
        onDismissed: message => {
            if (FanBackend.lastError === message)
                FanBackend.clearError()
            window.displayedError = ""
        }
    }

    Connections {
        target: FanBackend

        function onModeChanged() {
            window.finishModeTransitionIfReady()
        }

        function onModeRequestFinished(success, mode) {
            if (!window.modeTransitioning || mode !== window.pendingMode)
                return
            window.modeReplyFinished = true
            window.modeRequestSucceeded = success
            window.finishModeTransitionIfReady()
        }

        function onLastErrorChanged() {
            if (FanBackend.lastError.length > 0) {
                window.displayedError = FanBackend.lastError
                operationError.open()
            }
        }
    }
}
