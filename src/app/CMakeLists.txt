qt_add_executable(linux-thermal-control
    main.cpp
    TrayController.cpp
    TrayController.h
)

set_source_files_properties(qml/LinuxThermal/Control/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(linux-thermal-control
    URI LinuxThermal.Control
    VERSION 1.0
    SOURCES
        BackendClient.cpp
        BackendClient.h
        InventoryModels.cpp
        InventoryModels.h
    QML_FILES
        qml/LinuxThermal/Control/Main.qml
        qml/LinuxThermal/Control/Theme.qml
        qml/LinuxThermal/Control/Panel.qml
        qml/LinuxThermal/Control/MetricCard.qml
        qml/LinuxThermal/Control/ModeButton.qml
        qml/LinuxThermal/Control/AppHeader.qml
        qml/LinuxThermal/Control/AppFooter.qml
        qml/LinuxThermal/Control/Dashboard.qml
        qml/LinuxThermal/Control/ControlPage.qml
        qml/LinuxThermal/Control/InfoPage.qml
        qml/LinuxThermal/Control/HeaderNavigation.qml
        qml/LinuxThermal/Control/SecureBootPanel.qml
        qml/LinuxThermal/Control/MetricOverview.qml
        qml/LinuxThermal/Control/ModePanel.qml
        qml/LinuxThermal/Control/FanPanel.qml
        qml/LinuxThermal/Control/ModeConfirmDialog.qml
        qml/LinuxThermal/Control/OperationErrorDialog.qml
        qml/LinuxThermal/Control/CurveChart.qml
        qml/LinuxThermal/Control/CurveEditor.qml
        qml/LinuxThermal/Control/SafetyBanner.qml
        qml/LinuxThermal/Control/CapabilityBanner.qml
        qml/LinuxThermal/Control/BackendPanel.qml
)

target_include_directories(linux-thermal-control PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
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
