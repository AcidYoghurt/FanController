add_subdirectory(client)
add_subdirectory(qml/LinuxThermal/Control)

qt_add_executable(linux-thermal-control
    main.cpp
    platform/SingleInstanceController.cpp
    platform/SingleInstanceController.h
    platform/TrayController.cpp
    platform/TrayController.h
)

target_include_directories(linux-thermal-control PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(linux-thermal-control PRIVATE
    LinuxThermalControl::Common
    LinuxThermalControl::Client
    linux-thermal-clientplugin
    LinuxThermalControl::Ui
    linux-thermal-uiplugin
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

if(BUILD_TESTING)
    target_sources(linux-thermal-control PRIVATE
        ${PROJECT_SOURCE_DIR}/tests/app/GuiSmokeChecks.cpp
        ${PROJECT_SOURCE_DIR}/tests/app/GuiSmokeChecks.h
    )
    target_include_directories(linux-thermal-control PRIVATE
        ${PROJECT_SOURCE_DIR}/tests/app
    )
    target_compile_definitions(linux-thermal-control PRIVATE
        LINUX_THERMAL_GUI_TESTS
    )
endif()

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
