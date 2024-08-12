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

project(WifiControl)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

set(PLUGIN_WIFICONTROL_STARTMODE "Activated" CACHE STRING "Automatically start WifiControl plugin")
set(PLUGIN_WIFICONTROL_AUTOCONNECT true CACHE STRING "Automatically start to connect with configured SSID")
set(PLUGIN_WIFICONTROL_AUTOCONNECT_RETRYINTERVAL 30 CACHE STRING "Autoconnect Retry interval in seconds")
set(PLUGIN_WIFICONTROL_AUTOCONNECT_MAXRETRIES -1 CACHE STRING "Autoconnect max retries")
set(PLUGIN_WIFICONTROL_WPS_WALKTIME 125 CACHE STRING "Maximum Walk Time for WPS")
set(PLUGIN_WIFICONTROL_WPS_DISABLED false CACHE STRING "Disable the WPS Feature")

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED 
    WifiControl.cpp
    WifiControlImplementation.cpp
    Network.cpp      
    Module.cpp)

if (PLUGIN_USE_RDK_HAL_WIFI)
    target_compile_definitions(${MODULE_NAME} PRIVATE USE_WIFI_HAL)
    target_sources(${MODULE_NAME} PRIVATE WifiHAL.cpp)
else ()
    target_sources(${MODULE_NAME} PRIVATE Controller.cpp)
endif()

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_compile_options (${MODULE_NAME} PRIVATE -Wno-psabi)

target_link_libraries(${MODULE_NAME} 
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

install(TARGETS ${MODULE_NAME} 
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)

write_config()
