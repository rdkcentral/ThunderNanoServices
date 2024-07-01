# Copyright (c) 2022 Metrological. All rights reserved.

project(BackOffice)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})
set(PLUGIN_BACKOFFICE_STARTMODE "Activated" CACHE STRING "Automatically start the plugin")
set(PLUGIN_BACKOFFICE_SERVER_ADDRESS "localhost" CACHE STRING "Backoffice server address")
set(PLUGIN_BACKOFFICE_SERVER_PORT "80" CACHE STRING "Backoffice server address")
set(PLUGIN_BACKOFFICE_CUSTOMER "metrological" CACHE STRING "Customer name")
set(PLUGIN_BACKOFFICE_PLATFORM "rpi" CACHE STRING "Platform name")
set(PLUGIN_BACKOFFICE_COUNTRY "US" CACHE STRING "Country name")
set(PLUGIN_BACKOFFICE_TYPE "app" CACHE STRING "Type (default: app)")
set(PLUGIN_BACKOFFICE_SESSION 1234 CACHE STRING "Session number")
set(PLUGIN_BACKOFFICE_CALLSIGN_MAPPING "" CACHE STRING "Map framework callsigns in format: callsign1,mapping1;callsign2,mapping2")
set(PLUGIN_BACKOFFICE_STATE_MAPPING "Activated,load;Deactivated,unload;Resumed,open;Suspended,close" CACHE STRING "Map states in format: state1,mapping1;state2,mapping2, allowed states: Activated, Deactivated, Suspended, Resumed")

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Cryptalgo REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
    BackOffice.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Cryptalgo::${NAMESPACE}Cryptalgo)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)

write_config()
