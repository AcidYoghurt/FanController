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

    property string requestedMode: ""
    property string displayedError: ""
    property bool applyEditedCurve: false
    property int currentPage: 0

    header: AppHeader {
        connected: FanBackend.connected
        busy: FanBackend.busy
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
        onModeRequested: mode => {
            if (!FanBackend.canSetMode(mode))
                return
            window.applyEditedCurve = false
            window.requestedMode = mode
            confirmation.open()
        }
        onEditedCurveApplyRequested: {
            if (!FanBackend.connected || FanBackend.busy || !FanBackend.curveSupported)
                return
            window.applyEditedCurve = true
            window.requestedMode = "custom-curve"
            confirmation.open()
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

    ModeConfirmDialog {
        id: confirmation
        anchors.centerIn: parent
        requestedMode: window.requestedMode
        requestStillValid: window.applyEditedCurve
                           ? FanBackend.connected && !FanBackend.busy && FanBackend.curveSupported
                           : FanBackend.canSetMode(window.requestedMode)
        onModeConfirmed: mode => {
            if (window.applyEditedCurve)
                FanBackend.applyCurve()
            else
                FanBackend.setMode(mode)
            window.applyEditedCurve = false
        }
        onClosed: window.applyEditedCurve = false
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
        function onLastErrorChanged() {
            if (FanBackend.lastError.length > 0) {
                window.displayedError = FanBackend.lastError
                operationError.open()
            }
        }
    }
}
