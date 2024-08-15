# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2022 Metrological
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

project(Yin)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(${NAMESPACE}Messaging REQUIRED)
find_package(CompileSettingsDebug REQUIRED)

set(PLUGIN_YIN_IMPLEMENTATION "${MODULE_NAME}Impl" CACHE STRING "Library with the implementation of yin")

set(PLUGIN_YIN_STARTMODE "Deactivated" CACHE STRING "Controls automatic start of the Yin plugin")
set(PLUGIN_YIN_MODE "Local" CACHE STRING "Controls if the plugin should run in its own process, in process or remote")
set(PLUGIN_YIN_YANGCALLSIGN "Yang" CACHE STRING "Callsign of the Yang plugin")
set(PLUGIN_YIN_ETYMOLOGY "Bright side" CACHE STRING "Describes the meaning of yin")
set(PLUGIN_YIN_DATAFILE "symbol.txt" CACHE STRING "Name of the data file")

file(GLOB DATA_FILES "data/*")

add_subdirectory(interfaces)

add_library(${MODULE_NAME} SHARED
    Yin.cpp
    Module.cpp)

add_dependencies(${MODULE_NAME}
    ${PLUGIN_YIN_DEFINITIONS})

target_include_directories(${MODULE_NAME} SYSTEM
    PRIVATE
        ".")

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        CompileSettingsDebug::CompileSettingsDebug
        ${PLUGIN_YIN_DEFINITIONS})

add_library(${PLUGIN_YIN_IMPLEMENTATION} SHARED
    YinImplementation.cpp
    Module.cpp)

add_dependencies(${PLUGIN_YIN_IMPLEMENTATION}
    ${PLUGIN_YIN_DEFINITIONS})

target_include_directories(${PLUGIN_YIN_IMPLEMENTATION} SYSTEM
    PRIVATE
        "."
        "../Yang")

target_link_libraries(${PLUGIN_YIN_IMPLEMENTATION}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        CompileSettingsDebug::CompileSettingsDebug
        ${PLUGIN_YIN_DEFINITIONS})

install(TARGETS ${MODULE_NAME} DESTINATION ../${EXAMPLE_DYNAMICLOADING_TARGET}/usr/${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)
install(TARGETS ${PLUGIN_YIN_IMPLEMENTATION} DESTINATION ../${EXAMPLE_DYNAMICLOADING_TARGET}/usr/${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)
install(FILES ${DATA_FILES} DESTINATION ../${EXAMPLE_DYNAMICLOADING_TARGET}/root/${PLUGIN_NAME}  COMPONENT ${NAMESPACE}_Runtime)

write_config(INSTALL_PATH ../${EXAMPLE_DYNAMICLOADING_TARGET}/configs)
