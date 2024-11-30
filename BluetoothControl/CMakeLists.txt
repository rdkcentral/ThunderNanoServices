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
project(BluetoothControl)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

option(PLUGIN_BLUETOOTH_DEVELOPMENT "Enable verbose tracing" ON)
option(PLUGIN_BLUETOOTH_KERNEL_CONNECION_CONTROL "Enable kernel connection control" OFF)

set(PLUGIN_BLUETOOTH_STARTMODE "Activated" CACHE STRING "Automatically start the plugin")
set(PLUGIN_BLUETOOTH_PORT "/dev/ttyAMA0" CACHE STRING "Device port")
set(PLUGIN_BLUETOOTH_BAUDRATE 115200 CACHE STRING "Serial transmission speed in baud")
set(PLUGIN_BLUETOOTH_FIRMWARE_DIRECTORY "/etc/firmware" CACHE STRING "Chip firmware folder")
set(PLUGIN_BLUETOOTH_NAME "Thunder" CACHE STRING "Name of the local Bluetooth device")
set(PLUGIN_BLUETOOTH_SHORT_NAME "Thunder" CACHE STRING "Short name of the local Bluetooth device")
set(PLUGIN_BLUETOOTH_CLASS "0x000424" CACHE STRING "Class of the local Bluetooth device")
set(PLUGIN_BLUETOOTH_AUTO_PASSKEY_CONFIRM true CACHE STRING "Denotes if passkey confirmation request should be automattically cofirmed")
set(PLUGIN_BLUETOOTH_CONTINUOUS_BACKGROUND_SCAN false CACHE STRING "Denotes if background LE scan should be done continuously")
set(PLUGIN_BLUETOOTH_PERSISTMAC false CACHE STRING "Denotes Bluetooth address shall be made persistent")
set(PLUGIN_BLUETOOTH_SERVICE_A2DP_AUDIO_SOURCE "" CACHE STRING "Callsign of the A2DP Audio Source service (if any)")
set(PLUGIN_BLUETOOTH_SERVICE_A2DP_AUDIO_SINK "" CACHE STRING "Callsign of the A2DP Audio Sink service (if any)")
set(PLUGIN_BLUETOOTH_LE_CONNECTION_INTERVAL_MIN 8 CACHE STRING "LE min connection interval")
set(PLUGIN_BLUETOOTH_LE_CONNECTION_INTERVAL_MAX 8 CACHE STRING "LE max connection interval")
set(PLUGIN_BLUETOOTH_LE_CONNECTION_TIMEOUT 300 CACHE STRING "LE connection timeout")
set(PLUGIN_BLUETOOTH_LE_CONNECTION_LATENCY 0 CACHE STRING "LE connection latency")


if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

find_package(${NAMESPACE}Bluetooth REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
    BluetoothControl.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

if(PLUGIN_BLUETOOTH_DEVELOPMENT)
    target_compile_definitions(${MODULE_NAME}
        PRIVATE
            BLUETOOTH_DEVELOPMENT)
endif()

if(PLUGIN_BLUETOOTH_KERNEL_CONNECION_CONTROL)
    target_compile_definitions(${MODULE_NAME}
        PRIVATE
            USE_KERNEL_CONNECTION_CONTROL)
endif()

target_compile_options(${MODULE_NAME}
    PRIVATE
        -Wno-psabi
        -fdiagnostics-color=always)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ${NAMESPACE}Bluetooth::${NAMESPACE}Bluetooth)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)

write_config()
