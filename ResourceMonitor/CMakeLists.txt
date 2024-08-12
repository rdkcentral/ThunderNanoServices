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

set(PLUGIN_NAME ResourceMonitor)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
project(ResourceMonitor)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

set(PLUGIN_RESOURCEMONITOR_STARTMODE "Deactivated" CACHE STRING "Automatically start ResourceMonitor plugin")
set(PLUGIN_RESOURCEMONITOR_RESUMED "true" CACHE STRING "Set ResourceMonitor plugin resume state")
set(PLUGIN_RESOURCEMONITOR_MODE "Local" CACHE STRING "Controls if the plugin should run in its own process, in process or remote.")

# deprecated/legacy flags support
if(PLUGIN_RESOURCEMONITOR_OUTOFPROCESS STREQUAL "false")
    set(PLUGIN_RESOURCEMONITOR_MODE "Off" CACHE STRING "Process mode"  FORCE)
    unset(PLUGIN_RESOURCEMONITOR_OUTOFPROCESS CACHE)
endif()

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

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
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)

write_config()
