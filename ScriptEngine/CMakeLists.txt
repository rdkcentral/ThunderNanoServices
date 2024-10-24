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

project(ScriptEngine)

cmake_minimum_required(VERSION 3.3)

find_package(bridge)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})
set(PLUGIN_IMPLEMENTATION "${MODULE_NAME}Impl" CACHE STRING "library with NodeJS implementation." )

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED 
    ScriptEngine.cpp
    Module.cpp)

target_link_libraries(${MODULE_NAME} 
    PRIVATE 
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        )

# Select the proper script engine implementation..
add_subdirectory(Implementation/NodeJS)

add_library(${PLUGIN_IMPLEMENTATION} SHARED
    Module.cpp
    ScriptEngineImplementation.cpp)

target_link_libraries(${PLUGIN_IMPLEMENTATION}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        common::implementation)

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGENAME}/plugins)
install(TARGETS ${PLUGIN_IMPLEMENTATION} DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGENAME}/plugins)

set(PLUGIN_NODEJS_AUTOSTART "false" CACHE STRING "Automatically start Cobalt plugin")
set(PLUGIN_NODEJS_MODE "Local" CACHE STRING "Controls if the plugin should run in its own process, in process or remote.")
set(PLUGIN_NODEJS_RESUMED "true" CACHE STRING "Set Cobalt plugin resume state")

write_config(PLUGINS ${PROJECT_NAME})
