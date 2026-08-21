qt_add_executable(linux-thermal-daemon
    FanController.cpp
    FanController.h
    FanCurveConfig.cpp
    FanCurveConfig.h
    FanService.cpp
    FanService.h
    backends/IFanBackend.h
    backends/HwmonBackend.cpp
    backends/HwmonBackend.h
    backends/IdeaPadBackend.cpp
    backends/IdeaPadBackend.h
    backends/ClevoBackend.cpp
    backends/ClevoBackend.h
    backends/NvidiaBackend.cpp
    backends/NvidiaBackend.h
    backends/ThermalZoneBackend.cpp
    backends/ThermalZoneBackend.h
    backends/ThinkPadBackend.cpp
    backends/ThinkPadBackend.h
    backends/MockBackend.cpp
    backends/MockBackend.h
    main.cpp
)

target_include_directories(linux-thermal-daemon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

target_link_libraries(linux-thermal-daemon PRIVATE
    LinuxThermalControl::Common
    Qt6::Core
    Qt6::DBus
)

target_compile_definitions(linux-thermal-daemon PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    LINUX_THERMAL_DEFAULT_CURVE_SOURCE_PATH=\"${PROJECT_SOURCE_DIR}/config/default-fan-curve.yaml\"
)

install(TARGETS linux-thermal-daemon
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/linux-thermal-control
)
