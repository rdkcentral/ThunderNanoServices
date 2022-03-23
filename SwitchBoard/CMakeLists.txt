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

project(SwitchBoard)

cmake_minimum_required(VERSION 3.3)

find_package(WPEFramework)

set(PLUGIN_NAME SwitchBoard)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(PLUGIN_SWITCHBOARD_AUTOSTART "true" CACHE BOOL "Automatically start SwitchBoard plugin")
set(PLUGIN_SWITCHBOARD_DEFAULT ""  CACHE STRING "Callsign of the plugin to be activated in case nothing is active.")
set(PLUGIN_SWITCH_AMAZON false CACHE BOOL "Enable switch for Amazon plugin")
set(PLUGIN_SWITCH_COBALT false CACHE BOOL "Enable switch for Cobalt plugin")
set(PLUGIN_SWITCH_NETFLIX false CACHE BOOL "Enable switch for Netflix plugin")
set(PLUGIN_SWITCH_WEBKITBROWSER false CACHE BOOL "Enable switch for WebKitBrowser plugin")

# This is a plugin so we need the Plugin configuration section.
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
    SwitchBoard.cpp
    Module.cpp)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        CompileSettingsDebug::CompileSettingsDebug)

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config(${PLUGIN_NAME})
