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

project(FirmwareControl)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

find_package(MFRFWLibs REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

set(PLUGIN_FIRMWARECONTROL_STARTMODE "Deactivated" CACHE STRING "Automatically start FirmwareControl plugin")
set(PLUGIN_FIRMWARECONTROL_SOURCE_LOCATION "" CACHE STRING "Source URL or location of the firmware")
set(PLUGIN_FIRMWARECONTROL_DOWNLOAD_LOCATION "/tmp" CACHE STRING "Location where the firmware to be downloaded")
set(PLUGIN_FIRMWARECONTROL_WAITTIME -1 CACHE STRING "Max time to wait to finish download or install process")

add_library(${MODULE_NAME} SHARED
    FirmwareControl.cpp
    FirmwareControlJsonRpc.cpp
    Module.cpp
)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME} 
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        MFRFWLibs::MFRFWLibs
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
)

target_include_directories(${MODULE_NAME} 
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR})

install(TARGETS ${MODULE_NAME} 
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)

write_config()
