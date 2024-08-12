# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2020 Metrological
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

project(Cobalt)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})
set(PLUGIN_COBALT_IMPLEMENTATION "${MODULE_NAME}Impl" CACHE STRING "library with cobalt implementation." )

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(Cobalt REQUIRED)

get_filename_component(CONFIG_HEADER_NAME ${PLUGIN_COBALT_STARBOARD_CONFIGURATION_INCLUDE} NAME)
get_filename_component(CONFIG_HEADER_DIRECTORY ${PLUGIN_COBALT_STARBOARD_CONFIGURATION_INCLUDE} DIRECTORY)

find_file(CONFIG_HEADER  ${CONFIG_HEADER_NAME}
    PATH_SUFFIXES ${CONFIG_HEADER_DIRECTORY})

if(NOT CONFIG_HEADER)
    message(FATAL_ERROR "Cobalt configurtion not found: '${CONFIG_HEADER_DIRECTORY}/${CONFIG_HEADER_NAME}'")    
endif()

add_library(${MODULE_NAME} SHARED 
    Cobalt.cpp
    CobaltJsonRpc.cpp
    Module.cpp)

add_library(${PLUGIN_COBALT_IMPLEMENTATION} SHARED
    Module.cpp
    CobaltImplementation.cpp)

target_compile_definitions(${PLUGIN_COBALT_IMPLEMENTATION}
    PRIVATE 
        STARBOARD 
        STARBOARD_CONFIGURATION_INCLUDE="${PLUGIN_COBALT_STARBOARD_CONFIGURATION_INCLUDE}")

target_link_libraries(${MODULE_NAME} 
    PRIVATE 
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        )

target_link_libraries(${PLUGIN_COBALT_IMPLEMENTATION}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${COBALT_LIBRARIES}
        )

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGENAME}/plugins COMPONENT ${NAMESPACE}_Runtime)
install(TARGETS ${PLUGIN_COBALT_IMPLEMENTATION} DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGENAME}/plugins COMPONENT ${NAMESPACE}_Runtime) 

set(PLUGIN_COBALT_STARTMODE "Deactivated" CACHE STRING "Automatically start Cobalt plugin")
set(PLUGIN_COBALT_MODE "Local" CACHE STRING "Controls if the plugin should run in its own process, in process or remote.")
set(PLUGIN_COBALT_RESUMED "true" CACHE STRING "Set Cobalt plugin resume state")
set(PLUGIN_COBALT_RESOLUTION "720p" CACHE STRING "Browser resolution")
set(PLUGIN_COBALT_WEBINSPECTOR_ADDRESS 0.0.0.0:9222 CACHE STRING "IP:Port for WebInspector of Cobalt")

# deprecated/legacy flags support
if(PLUGIN_COBALT_OUTOFPROCESS STREQUAL "false")
    set(PLUGIN_COBALT_MODE "Off" CACHE STRING "Process mode"  FORCE)
    unset(PLUGIN_COBALT_OUTOFPROCESS CACHE)
endif()

# resolution handling
if(PLUGIN_COBALT_RESOLUTION EQUAL "720p")
    set(PLUGIN_COBALT_HEIGHT "720")
    set(PLUGIN_COBALT_WIDTH "1280")
elseif(PLUGIN_COBALT_RESOLUTION EQUAL "1080p")
    set(PLUGIN_COBALT_HEIGHT "1080")
    set(PLUGIN_COBALT_WIDTH "1920")
elseif(PLUGIN_COBALT_RESOLUTION EQUAL "2160p")
    set(PLUGIN_COBALT_HEIGHT "2160")
    set(PLUGIN_COBALT_WIDTH "3840")
endif()

write_config()
