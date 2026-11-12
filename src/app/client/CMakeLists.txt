qt_add_library(linux-thermal-client STATIC)
add_library(LinuxThermalControl::Client ALIAS linux-thermal-client)

qt_add_qml_module(linux-thermal-client
    URI LinuxThermal.Backend
    VERSION 1.0
    OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/qml/LinuxThermal/Backend
    SOURCES
        BackendClient.cpp
        BackendClient.h
        ../models/InventoryModels.cpp
        ../models/InventoryModels.h
)

target_include_directories(linux-thermal-client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(linux-thermal-client
    PUBLIC
        Qt6::Core
        Qt6::DBus
        Qt6::Qml
    PRIVATE
        LinuxThermalControl::Common
)

target_compile_definitions(linux-thermal-client PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)
