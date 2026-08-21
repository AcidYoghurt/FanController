set(THERMAL_BACKEND_SOURCES
    ${PROJECT_SOURCE_DIR}/src/daemon/FanController.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/FanController.h
    ${PROJECT_SOURCE_DIR}/src/daemon/FanCurveConfig.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/FanCurveConfig.h
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/IFanBackend.h
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/HwmonBackend.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/HwmonBackend.h
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/IdeaPadBackend.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/IdeaPadBackend.h
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/ClevoBackend.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/ClevoBackend.h
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/NvidiaBackend.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/NvidiaBackend.h
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/ThermalZoneBackend.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/ThermalZoneBackend.h
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/ThinkPadBackend.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/ThinkPadBackend.h
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/MockBackend.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/MockBackend.h
)

qt_add_executable(linux-thermal-tests
    FanControllerTest.cpp
    ${THERMAL_BACKEND_SOURCES}
)

target_include_directories(linux-thermal-tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/daemon
    ${PROJECT_SOURCE_DIR}/src/daemon/backends
)

target_link_libraries(linux-thermal-tests PRIVATE
    LinuxThermalControl::Common
    Qt6::Core
    Qt6::Test
)

target_compile_definitions(linux-thermal-tests PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    LINUX_THERMAL_DEFAULT_CURVE_SOURCE_PATH=\"${PROJECT_SOURCE_DIR}/config/default-fan-curve.yaml\"
)

qt_add_executable(temperature-sensor-selector-tests
    TemperatureSensorSelectorTest.cpp
)

target_link_libraries(temperature-sensor-selector-tests PRIVATE
    LinuxThermalControl::Common
    Qt6::Core
    Qt6::Test
)

target_compile_definitions(temperature-sensor-selector-tests PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)

qt_add_executable(hwmon-backend-tests
    HwmonBackendTest.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/HwmonBackend.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/HwmonBackend.h
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/IFanBackend.h
)

target_include_directories(hwmon-backend-tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/daemon/backends
)

target_link_libraries(hwmon-backend-tests PRIVATE
    LinuxThermalControl::Common
    Qt6::Core
    Qt6::Test
)

target_compile_definitions(hwmon-backend-tests PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)


qt_add_executable(platform-backend-tests
    PlatformBackendTest.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/ThermalZoneBackend.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/ThermalZoneBackend.h
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/ThinkPadBackend.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/ThinkPadBackend.h
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/IdeaPadBackend.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/IdeaPadBackend.h
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/IFanBackend.h
)

target_include_directories(platform-backend-tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/daemon/backends
)

target_link_libraries(platform-backend-tests PRIVATE
    LinuxThermalControl::Common
    Qt6::Core
    Qt6::Test
)

target_compile_definitions(platform-backend-tests PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)


add_library(fake-nvml SHARED
    FakeNvml.cpp
)
set_target_properties(fake-nvml PROPERTIES
    OUTPUT_NAME nvidia-ml
    PREFIX lib
)

qt_add_executable(nvidia-backend-tests
    NvidiaBackendTest.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/NvidiaBackend.cpp
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/NvidiaBackend.h
    ${PROJECT_SOURCE_DIR}/src/daemon/backends/IFanBackend.h
)

target_include_directories(nvidia-backend-tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/daemon/backends
)

target_link_libraries(nvidia-backend-tests PRIVATE
    LinuxThermalControl::Common
    Qt6::Core
    Qt6::Test
)

target_compile_definitions(nvidia-backend-tests PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)


qt_add_executable(tray-controller-tests
    TrayControllerTest.cpp
    ${PROJECT_SOURCE_DIR}/src/app/TrayController.cpp
    ${PROJECT_SOURCE_DIR}/src/app/TrayController.h
)

target_include_directories(tray-controller-tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/app
)

target_link_libraries(tray-controller-tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    Qt6::Widgets
)

target_compile_definitions(tray-controller-tests PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)

qt_add_executable(linux-thermal-dbus-tests
    MockDbusIntegrationTest.cpp
)

target_link_libraries(linux-thermal-dbus-tests PRIVATE
    LinuxThermalControl::Common
    Qt6::Core
    Qt6::DBus
    Qt6::Test
)

target_compile_definitions(linux-thermal-dbus-tests PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)

add_test(NAME linux-thermal-unit-tests COMMAND linux-thermal-tests)
add_test(NAME temperature-sensor-selector-unit-tests COMMAND temperature-sensor-selector-tests)
add_test(NAME hwmon-backend-unit-tests COMMAND hwmon-backend-tests)
add_test(NAME platform-backend-unit-tests COMMAND platform-backend-tests)
add_test(NAME nvidia-backend-unit-tests COMMAND nvidia-backend-tests)
add_test(NAME tray-controller-unit-tests COMMAND tray-controller-tests)
set_tests_properties(tray-controller-unit-tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
set_tests_properties(nvidia-backend-unit-tests PROPERTIES
    ENVIRONMENT "THERMAL_CONTROL_NVML_LIBRARY=$<TARGET_FILE:fake-nvml>"
)
add_test(NAME packaging-maintainer-scripts-tests
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/PackagingScriptsTest.sh
)

add_test(NAME linux-thermal-single-instance-tests
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run-single-instance-test.sh
            $<TARGET_FILE:linux-thermal-control>
)

add_test(NAME linux-thermal-dbus-integration-tests
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run-mock-dbus-test.sh
            $<TARGET_FILE:linux-thermal-daemon>
            $<TARGET_FILE:linux-thermal-dbus-tests>
)
