qt_add_library(linux-thermal-ui STATIC)
add_library(LinuxThermalControl::Ui ALIAS linux-thermal-ui)

set_source_files_properties(
    Theme.qml
    state/AppSettings.qml
    PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(linux-thermal-ui
    URI LinuxThermal.Control
    VERSION 1.0
    OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/qml/LinuxThermal/Control
    DEPENDENCIES TARGET linux-thermal-client
    QML_FILES
        Main.qml
        Theme.qml
        state/AppController.qml
        state/AppSettings.qml
        pages/ControlPage.qml
        pages/Dashboard.qml
        pages/InfoPage.qml
        components/CurvePointRow.qml
        components/CurveSpinBox.qml
        components/SelectField.qml
        components/ActionButton.qml
        components/PageHeading.qml
        components/ThermalIcon.qml
        components/AppFooter.qml
        components/AppHeader.qml
        components/CapabilityBanner.qml
        components/CurveChart.qml
        components/HeaderNavigation.qml
        components/MetricCard.qml
        components/ModeButton.qml
        components/Panel.qml
        components/SafetyBanner.qml
        panels/BackendPanel.qml
        panels/CurveEditor.qml
        panels/FanPanel.qml
        panels/MetricOverview.qml
        panels/ModePanel.qml
        panels/SecureBootPanel.qml
        dialogs/AppearancePopup.qml
        dialogs/OperationErrorDialog.qml
)

target_link_libraries(linux-thermal-ui PUBLIC
    Qt6::Quick
    Qt6::QuickControls2
)
