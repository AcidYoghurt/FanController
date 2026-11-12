import QtQuick
import QtQuick.Controls
import LinuxThermal.Control
import LinuxThermal.Backend

ApplicationWindow {
    id: window
    width: 1120
    height: Math.min(900, Screen.desktopAvailableHeight - 80)
    minimumWidth: 760
    minimumHeight: 620
    visible: true
    title: qsTr("Linux Thermal Control")
    color: Theme.window

    font.pixelSize: Theme.bodySize
    palette.window: Theme.window
    palette.windowText: Theme.text
    palette.base: Theme.inset
    palette.alternateBase: Theme.panelHover
    palette.text: Theme.text
    palette.button: Theme.panel
    palette.buttonText: Theme.text
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentForeground
    palette.mid: Theme.borderStrong
    palette.dark: Theme.border
    palette.light: Theme.panelHover

    property alias currentPage: controller.currentPage

    AppController {
        id: controller
        backend: FanBackend
        onErrorRaised: operationError.open()
    }

    header: AppHeader {
        connected: FanBackend.connected
        busy: FanBackend.busy && !controller.modeTransitioning
        refreshing: FanBackend.refreshing
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
        modeTransitioning: controller.modeTransitioning
        onModeRequested: mode => controller.requestMode(mode)
        onEditedCurveApplyRequested: controller.applyEditedCurve()
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
        errorMessage: controller.displayedError
        onDismissed: message => controller.dismissError(message)
    }
}
