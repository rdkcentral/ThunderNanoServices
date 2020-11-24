# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
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

cmake_minimum_required(VERSION 3.3)

project(Cobalt)

find_package(WPEFramework)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(PLUGIN_NAME Cobalt)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
set(PLUGIN_COBALT_WEBINSPECTOR_ADDRESS 0.0.0.0:9222 CACHE STRING "IP:Port for WebInspector of Cobalt")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(Cobalt REQUIRED)

get_filename_component(CONFIG_HEADER_NAME ${WPEFRAMEWORK_PLUGIN_COBALT_STARBOARD_CONFIGURATION_INCLUDE} NAME)
get_filename_component(CONFIG_HEADER_DIRECTORY ${WPEFRAMEWORK_PLUGIN_COBALT_STARBOARD_CONFIGURATION_INCLUDE} DIRECTORY)

find_file(CONFIG_HEADER  ${CONFIG_HEADER_NAME}
    PATH_SUFFIXES ${CONFIG_HEADER_DIRECTORY})

if(NOT CONFIG_HEADER)
    message(FATAL_ERROR "Cobalt configurtion not found: '${CONFIG_HEADER_DIRECTORY}/${CONFIG_HEADER_NAME}'")    
endif()

add_library(${MODULE_NAME} SHARED 
    Cobalt.cpp
    CobaltImplementation.cpp
    CobaltJsonRpc.cpp
    Module.cpp)

target_compile_definitions(${MODULE_NAME} 
    PRIVATE 
        STARBOARD 
        STARBOARD_CONFIGURATION_INCLUDE="${WPEFRAMEWORK_PLUGIN_COBALT_STARBOARD_CONFIGURATION_INCLUDE}")

target_link_libraries(${MODULE_NAME} 
    PRIVATE 
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${COBALT_LIBRARIES}
        )

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

set(PLUGIN_COBALT_AUTOSTART false CACHE STRING "Automatically start Cobalt plugin")
set(PLUGIN_COBALT_OUTOFPROCESS false CACHE STRING "Controls if the plugin should run in its own process")
set(PLUGIN_COBALT_RESOLUTION "720p" CACHE STRING "Browser resolution")

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

write_config(${PLUGIN_NAME})
