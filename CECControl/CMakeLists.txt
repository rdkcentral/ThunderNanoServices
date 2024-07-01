# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2021 Metrological
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

project(CECControl)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(PLUGIN_CECCONTROL_STARTMODE "Activated" CACHE STRING "Automatically start CECControl plugin")

# TODO: CEC 2.0 specifies that multiple roles can be used by one device.
set(PLUGIN_CECCONTROL_ROLES "playback" CACHE STRING "Device type")
set(PLUGIN_CECCONTROL_NAME "Thunder" CACHE STRING "The name of this device")

set(CECADAPTER_LIBRARY_NAME "_cecadapter" CACHE STRING "Defines the name of the library")
set(CEC_DEVICE_ADAPTER_LIBRARY_NAME "_ceccontrol_device_adapter" CACHE INTERNAL "INTERNALLY USED")

set (PLUGIN_CECCONTROL_INTERFACES "hdmi1:/dev/cec0:recorder" "hdmi2:/dev/cec1:playback")

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}COM REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

#[[ 
    Enable PIC by default for this project to fix 
    dangerous relocation of symbols.
#]]
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(simpleworker)
add_subdirectory(adapter)

add_library(${MODULE_NAME} SHARED
    CECControl.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${CECADAPTER_LIBRARY_NAME}
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

target_include_directories(${MODULE_NAME}
    PRIVATE
        ${CEC_INCLUDE_DIR})

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)

write_config()
