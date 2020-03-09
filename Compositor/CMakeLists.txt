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

set(PLUGIN_NAME Compositor)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
set(PLATFORM_COMPOSITOR "compositorplugin")

option(PLUGIN_COMPOSITOR_SYSTEM "Callsign to report information." "Controller")
option(PLUGIN_COMPOSITOR_IMPLEMENTATION "Defines which implementation is used." "None")
option(PLUGIN_COMPOSITOR_HARDWAREREADY "Manual overrule of the time it takes to initialisize all hardware (s) [0 - UINT16_MAX]." 0)
option(PLUGIN_COMPOSITOR_SERVICE "Defines where to get resouce handles. [internal, external]." external)
option(PLUGIN_COMPOSITOR_OUTOFPROCESS "Controls if the plugin should run in its own process. [true, false]." true)
option(PLUGIN_COMPOSITOR_AUTOTRACE "Contols if the plugin should automatically start tracing. [ON, OFF]." OFF)
option(PLUGIN_COMPOSITOR_BOXMODE "Allows for selecting a boxmode (Nexus only).")
option(PLUGIN_COMPOSITOR_GRAPHICS_HEAP_SIZE "Change graphic heap of driver (Nexus only).")

option(PLUGIN_COMPOSITOR_TEST "Build a compositor test client" OFF)

set(PLUGIN_COMPOSITOR_IMPLEMENTATION_LIB "lib${PLATFORM_COMPOSITOR}.so" CACHE STRING "Specify a library with a compositor implentation." )
set(PLUGIN_COMPOSITOR_RESOLUTION "720p" CACHE STRING "Specify the startup resolution")

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_REVISION 0)

set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION})

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

write_config(${PLUGIN_NAME})
