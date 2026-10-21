project(TestSmartProvider)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
    TestSmartProvider.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
    CXX_STANDARD ${CXX_STD}
    CXX_STANDARD_REQUIRED YES
    LIBRARY_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/ThunderNanoServices/tests/TestPluginSmartInterface")

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        CompileSettingsDebug::CompileSettingsDebug)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins
    COMPONENT ${NAMESPACE}_Runtime)

set(PLUGIN_TESTSMARTPROVIDER_STARTMODE "Deactivated" CACHE STRING
    "Startup mode of the TestSmartProvider plugin")

set(PLUGIN_TESTSMARTPROVIDER_RESUMED false CACHE BOOL
    "Start TestSmartProvider in resumed state")

write_config()
