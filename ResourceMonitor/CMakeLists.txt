project(ResourceMonitor)

cmake_minimum_required(VERSION 3.3)

find_package(WPEFramework)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

set(PLUGIN_RESOURCEMONITOR_AUTOSTART false CACHE STRING "Automatically start ResourceMonitor plugin")
set(PLUGIN_RESOURCEMONITOR_RESUMED true CACHE STRING "Set ResourceMonitor plugin resume state")
set(PLUGIN_RESOURCEMONITOR_OUTOFPROCESS true CACHE STRING "Controls if the plugin should run in its own process")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED 
    ResourceMonitor.cpp
    ResourceMonitorImplementation.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME} 
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

install(TARGETS ${MODULE_NAME} 
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config()
