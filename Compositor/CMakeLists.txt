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

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

set(PLATFORM_COMPOSITOR "compositorplugin")

option(PLUGIN_COMPOSITOR_IMPLEMENTATION "Defines which implementation is used." "None")
option(PLUGIN_COMPOSITOR_HARDWAREREADY "Manual overrule of the time it takes to initialisize all hardware (s) [0 - UINT16_MAX]." 0)

set(PLUGIN_COMPOSITOR_STARTMODE "Activated" CACHE STRING "Automatically start Compositor plugin")
set(PLUGIN_COMPOSITOR_MODE "Local" CACHE STRING "Controls if the plugin should run in its own process, in process or remote.")

option(PLUGIN_COMPOSITOR_BOXMODE "Allows for selecting a boxmode (Nexus only).")
option(PLUGIN_COMPOSITOR_GRAPHICS_HEAP_SIZE "Change graphic heap of driver (Nexus only).")
option(PLUGIN_COMPOSITOR_ALLOW_UNAUTHENTICATED_CLIENTS "Disable nxserver client authentication." OFF)

set(PLUGIN_COMPOSITOR_SAGE_PATH "/usr/bin" CACHE STRING "Specifies the location of the SAGE binaries")
set(PLUGIN_COMPOSITOR_PAK_PATH "/usr/bin" CACHE STRING "Specifies the location of the PAK binaries")
set(PLUGIN_COMPOSITOR_DRM_PATH "/usr/bin" CACHE STRING "Specifies the location of the DRM binaries")
set(PLUGIN_COMPOSITOR_HDCP1XBIN_PATH "/usr/bin" CACHE STRING "Specifies the location of the HDCP 1 binaries")
set(PLUGIN_COMPOSITOR_HDCP2XBIN_PATH "/usr/bin" CACHE STRING "Specifies the location of the HDCP 2 binaries")
set(PLUGIN_COMPOSITOR_SVP "None" CACHE STRING "SVP pipeline type")

option(PLUGIN_COMPOSITOR_FAIL_SAFE_HDR "Enable fail safe HDR values" OFF)

option(PLUGIN_COMPOSITOR_TEST "Build a compositor test client" OFF)
option(PLUGIN_COMPOSITOR_NXSERVER "Include a nexus server (Used in Wayland)" OFF)

set(PLUGIN_COMPOSITOR_IMPLEMENTATION_LIB "lib${PLATFORM_COMPOSITOR}.so" CACHE STRING "Specify a library with a compositor implementation." )
set(PLUGIN_COMPOSITOR_RESOLUTION "720p" CACHE STRING "Specify the startup resolution")

set(PLUGIN_COMPOSITOR_OUTPUTS "HDMI-A-1;0;0;1080;1920"  CACHE STRING "Specify the outputs (name1;x;y;h;w|name2;x;y;h;w) that the compositor should use." )
set(PLUGIN_COMPOSITOR_CANVAS_HEIGHT "720"  CACHE STRING "Specify the height of the canvas that the compositor uses for drawing." )
set(PLUGIN_COMPOSITOR_CANVAS_WIDTH "1280"  CACHE STRING "Specify the width of the canvas that the compositor uses for drawing." )
set(PLUGIN_COMPOSITOR_BUFFER_CONNECTOR "buffer|775"  CACHE STRING "Specify the width of the canvas that the compositor uses for drawing." )
set(PLUGIN_COMPOSITOR_DISPLAY_CONNECTOR "display|775"  CACHE STRING "Specify the width of the canvas that the compositor uses for drawing." )
set(PLUGIN_COMPOSITOR_RENDER_NODE ""  CACHE STRING "Manually specify the render node to use (e.g. /dev/dri/renderDxxx or /dev/dri/cardx)." )

set(PLUGIN_COMPOSITOR_WESTON_TTY_LIST "1;2;3;4" CACHE STRING "TTY ids for weston drm backend")
set(PLUGIN_COMPOSITOR_WESTON_OUTPUT_CONFIGS "HDMI-A-1,1280x720@60.0 16:9,normal" CACHE STRING "Output configs for weston drm backend")
set(PLUGIN_COMPOSITOR_THREAD_COUNT "2"  CACHE STRING "Specify the minimum thread count required to run in outofprocess mode")

set(PLUGIN_COMPOSITOR_STARTMODE "Activated" CACHE STRING "The startup state of the plugin")

# deprecated/legacy flags support
if(PLUGIN_COMPOSITOR_OUTOFPROCESS STREQUAL "false")
    set(PLUGIN_COMPOSITOR_MODE "Off" CACHE STRING "Process mode"  FORCE)
    unset(PLUGIN_COMPOSITOR_OUTOFPROCESS CACHE)
endif()

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

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

if(PLUGIN_COMPOSITOR_NXSERVER)
    target_compile_definitions(${MODULE_NAME} 
        PRIVATE
            PROVIDES_PLATORM)
endif()

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)


add_subdirectory(lib)

add_library(platform::implementation ALIAS ${PLATFORM_COMPOSITOR})

add_subdirectory(test)

write_config()
