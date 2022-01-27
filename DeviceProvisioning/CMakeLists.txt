project(DeviceProvisioning)
cmake_minimum_required(VERSION 3.3)

find_package(WPEFramework)
find_package(${NAMESPACE}Plugins REQUIRED)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})
message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

set(PLUGIN_DEVICEPROVISIONING_AUTOSTART "false" CACHE STRING "Automatically start DeviceProvisioning plugin")
set(PLUGIN_DEVICEPROVISIONING_MODE "Local" CACHE STRING "Controls if the plugin should run in its own process, in process or remote.")

add_library(${MODULE_NAME} SHARED 
    Module.cpp
    DeviceProvisioning.cpp
    DeviceProvisioningImplementation.cpp)

target_link_libraries(${MODULE_NAME} 
    PRIVATE 
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        )

# Installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config()
