qt_add_executable(linux-thermal-control
    main.cpp
    platform/SingleInstanceController.cpp
    platform/SingleInstanceController.h
    platform/TrayController.cpp
    platform/TrayController.h
)

set_source_files_properties(qml/LinuxThermal/Control/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(linux-thermal-control
    URI LinuxThermal.Control
    VERSION 1.0
    SOURCES
        client/BackendClient.cpp
        client/BackendClient.h
        models/InventoryModels.cpp
        models/InventoryModels.h
    QML_FILES
        qml/LinuxThermal/Control/Main.qml
        qml/LinuxThermal/Control/Theme.qml
        qml/LinuxThermal/Control/pages/ControlPage.qml
        qml/LinuxThermal/Control/pages/Dashboard.qml
        qml/LinuxThermal/Control/pages/InfoPage.qml
        qml/LinuxThermal/Control/components/AppFooter.qml
        qml/LinuxThermal/Control/components/AppHeader.qml
        qml/LinuxThermal/Control/components/CapabilityBanner.qml
        qml/LinuxThermal/Control/components/CurveChart.qml
        qml/LinuxThermal/Control/components/HeaderNavigation.qml
        qml/LinuxThermal/Control/components/MetricCard.qml
        qml/LinuxThermal/Control/components/ModeButton.qml
        qml/LinuxThermal/Control/components/Panel.qml
        qml/LinuxThermal/Control/components/SafetyBanner.qml
        qml/LinuxThermal/Control/panels/BackendPanel.qml
        qml/LinuxThermal/Control/panels/CurveEditor.qml
        qml/LinuxThermal/Control/panels/FanPanel.qml
        qml/LinuxThermal/Control/panels/MetricOverview.qml
        qml/LinuxThermal/Control/panels/ModePanel.qml
        qml/LinuxThermal/Control/panels/SecureBootPanel.qml
        qml/LinuxThermal/Control/dialogs/OperationErrorDialog.qml
)

target_include_directories(linux-thermal-control PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/client
    ${CMAKE_CURRENT_SOURCE_DIR}/models
)

target_link_libraries(linux-thermal-control PRIVATE
    LinuxThermalControl::Common
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
)

target_compile_definitions(linux-thermal-control PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)

install(TARGETS linux-thermal-control
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET linux-thermal-control
    OUTPUT_SCRIPT linux_thermal_deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    EXCLUDE_PLUGIN_TYPES qmltooling
)
install(SCRIPT ${linux_thermal_deploy_script})
