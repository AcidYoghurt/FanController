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
            if (FanBackend.canSetMode(mode))
                FanBackend.setMode(mode)
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
        function onLastErrorChanged() {
            if (FanBackend.lastError.length > 0) {
                window.displayedError = FanBackend.lastError
                operationError.open()
            }
        }
    }
}
