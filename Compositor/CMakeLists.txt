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
project(Compositor)

cmake_minimum_required(VERSION 3.3)

find_package(WPEFramework)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

set(PLATFORM_COMPOSITOR "compositorplugin")

option(PLUGIN_COMPOSITOR_IMPLEMENTATION "Defines which implementation is used." "None")
option(PLUGIN_COMPOSITOR_HARDWAREREADY "Manual overrule of the time it takes to initialisize all hardware (s) [0 - UINT16_MAX]." 0)
option(PLUGIN_COMPOSITOR_OUTOFPROCESS "Controls if the plugin should run in its own process. [true, false]." true)
option(PLUGIN_COMPOSITOR_BOXMODE "Allows for selecting a boxmode (Nexus only).")
option(PLUGIN_COMPOSITOR_GRAPHICS_HEAP_SIZE "Change graphic heap of driver (Nexus only).")
set(PLUGIN_COMPOSITOR_FAIL_SAFE_HDR OFF CACHE BOOL "Enable fail safe HDR values")

option(PLUGIN_COMPOSITOR_TEST "Build a compositor test client" OFF)
option(PLUGIN_COMPOSITOR_NXSERVER "Allow to use external nexus server" OFF)

set(PLUGIN_COMPOSITOR_IMPLEMENTATION_LIB "lib${PLATFORM_COMPOSITOR}.so" CACHE STRING "Specify a library with a compositor implentation." )
set(PLUGIN_COMPOSITOR_RESOLUTION "720p" CACHE STRING "Specify the startup resolution")
set(PLUGIN_COMPOSITOR_WESTON_TTY_LIST "1;2;3;4" CACHE STRING "TTY ids for weston drm backend")
set(PLUGIN_COMPOSITOR_WESTON_OUTPUT_CONFIGS "HDMI-A-1,1280x720@60.0 16:9,normal" CACHE STRING "Output configs for weston drm backend")


find_package(${NAMESPACE}Definitions REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED 
    Module.cpp
    Compositor.cpp
    CompositorJsonRpc.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME} 
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

if(PLUGIN_COMPOSITOR_SERVER)
    add_subdirectory(server)
endif ()

add_subdirectory(lib)

write_config()
