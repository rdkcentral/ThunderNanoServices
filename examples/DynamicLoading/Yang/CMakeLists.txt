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

project(Yang)

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

set(PLUGIN_YANG_IMPLEMENTATION "${MODULE_NAME}Impl" CACHE STRING "Library with implementation of yang")

set(PLUGIN_YANG_STARTMODE "Deactivated" CACHE STRING "Controls automatic start of the Yang plugin")
set(PLUGIN_YANG_MODE "Local" CACHE STRING "Controls if the plugin should run in its own process, in process or remote")
set(PLUGIN_YANG_YINCALLSIGN "Yin" CACHE STRING "Callsign of the Yin plugin")
set(PLUGIN_YANG_ETYMOLOGY "Dark side" CACHE STRING "Describes the meaning of yang")
set(PLUGIN_YANG_COLOR "Black" CACHE STRING "Default color of yang")

file(GLOB DATA_FILES "data/*")

add_subdirectory(interfaces)

add_library(${MODULE_NAME} SHARED
    Yang.cpp
    Module.cpp)

add_dependencies(${MODULE_NAME}
    ${PLUGIN_YANG_DEFINITIONS})

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
        ${PLUGIN_YANG_DEFINITIONS})

add_library(${PLUGIN_YANG_IMPLEMENTATION} SHARED
    YangImplementation.cpp
    Module.cpp)

add_dependencies(${PLUGIN_YANG_IMPLEMENTATION}
    ${PLUGIN_YANG_DEFINITIONS})

target_include_directories(${PLUGIN_YANG_IMPLEMENTATION} SYSTEM
    PRIVATE
        "."
        "../Yin")

target_link_libraries(${PLUGIN_YANG_IMPLEMENTATION}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        CompileSettingsDebug::CompileSettingsDebug
        ${PLUGIN_YANG_DEFINITIONS})

# Library installation section
install(TARGETS ${MODULE_NAME} DESTINATION ../${EXAMPLE_DYNAMICLOADING_TARGET}/usr/${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)
install(TARGETS ${PLUGIN_YANG_IMPLEMENTATION} DESTINATION ../${EXAMPLE_DYNAMICLOADING_TARGET}/usr/${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)

write_config(INSTALL_PATH ../${EXAMPLE_DYNAMICLOADING_TARGET}/configs)
