add_library(linux-thermal-common INTERFACE)
add_library(LinuxThermalControl::Common ALIAS linux-thermal-common)

target_sources(linux-thermal-common INTERFACE
    Constants.h
    DbusUtils.h
    ThermalTypes.h
    TemperatureSensorSelector.h
)

target_include_directories(linux-thermal-common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
