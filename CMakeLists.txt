cmake_minimum_required(VERSION 3.22)

project(LinuxThermalControl
    VERSION 0.5.13
    DESCRIPTION "Capability-aware Linux thermal and fan control"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.11.2 REQUIRED COMPONENTS
    Core
    DBus
    Gui
    Qml
    Quick
    QuickControls2
    Widgets
)

qt_standard_project_setup(REQUIRES 6.11.2)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.11.2 REQUIRED COMPONENTS Test)
endif()

add_subdirectory(src/common)
add_subdirectory(src/daemon)
add_subdirectory(src/app)
add_subdirectory(packaging)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
