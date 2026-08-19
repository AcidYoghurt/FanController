import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ScrollView {
    id: root
    objectName: "controlPage"

    required property var backend
    signal modeRequested(string mode)
    signal editedCurveApplyRequested

    contentWidth: availableWidth
    clip: true

    ColumnLayout {
        width: root.availableWidth
        spacing: Theme.sectionSpacing

        Item { implicitHeight: 4 }

        CapabilityBanner {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.pageMargin
            Layout.rightMargin: Theme.pageMargin
            connected: root.backend.connected
            backendState: root.backend.state
            partialCapability: root.backend.partialCapability
        }

        SafetyBanner {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.pageMargin
            Layout.rightMargin: Theme.pageMargin
            active: root.backend.safetyOverride
            message: root.backend.safetyMessage
        }

        MetricOverview {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.pageMargin
            Layout.rightMargin: Theme.pageMargin
            sensorModel: root.backend.sensors
        }

        ModePanel {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.pageMargin
            Layout.rightMargin: Theme.pageMargin
            currentMode: root.backend.mode
            automaticAvailable: root.backend.connected && !root.backend.busy
                                && root.backend.automaticAvailable
            fullSpeedAvailable: root.backend.connected && !root.backend.busy
                                && root.backend.fullSpeedAvailable
            customCurveAvailable: root.backend.connected && !root.backend.busy
                                  && root.backend.customCurveAvailable
            onModeRequested: mode => root.modeRequested(mode)
        }

        CurveEditor {
            id: curveEditor
            Layout.fillWidth: true
            Layout.leftMargin: Theme.pageMargin
            Layout.rightMargin: Theme.pageMargin
            zoneModel: root.backend.zones
            curveModel: root.backend.curvePoints
            selectedZoneId: root.backend.selectedZoneId
            selectedZoneName: root.backend.selectedZoneName
            minimumDuty: root.backend.minimumDuty
            safetyTemperature: root.backend.safetyTemperature
            currentTemperature: root.backend.selectedTemperature
            currentDuty: root.backend.selectedDuty
            curveSupported: root.backend.curveSupported
            connected: root.backend.connected
            busy: root.backend.busy
            conflict: root.backend.curveConflict
            onZoneRequested: zoneId => {
                if (!root.backend.selectZone(zoneId))
                    curveEditor.syncZoneSelection()
            }
            onPointEdited: (row, temperature, duty) =>
                               root.backend.setCurvePoint(row, temperature, duty)
            onSaveRequested: root.backend.saveCurve()
            onApplyRequested: root.editedCurveApplyRequested()
            onResetRequested: root.backend.resetCurve()
            onDiscardRequested: root.backend.discardCurveEdits()
        }

        Item { implicitHeight: 16 }
    }
}
