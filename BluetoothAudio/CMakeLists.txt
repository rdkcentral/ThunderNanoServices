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

project(BluetoothAudio)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)
find_package(${NAMESPACE}Bluetooth REQUIRED)
find_package(${NAMESPACE}BluetoothAudio REQUIRED)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

find_package(${NAMESPACE}Bluetooth REQUIRED)
find_package(${NAMESPACE}BluetoothAudio REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

set(PLUGIN_BLUETOOTHAUDIO_STARTMODE "Activated" CACHE STRING "Automatically start the plugin")
set(PLUGIN_BLUETOOTHAUDIO_CONTROLLER "BluetoothControl" CACHE STRING "Bluetooth controller callsign")
set(PLUGIN_BLUETOOTHAUDIO_INTERFACE "0" CACHE STRING "Bluetooth host interface to use")
set(PLUGIN_BLUETOOTHAUDIO_INACTIVITYTIMEOUT "5000" CACHE STRING "Server inactivity timeout")
set(PLUGIN_BLUETOOTHAUDIO_SINK_LATENCY "20" CACHE STRING "Audio sink device audio latency")
set(PLUGIN_BLUETOOTHAUDIO_SINK_CODECSBC_PRESET "HQ" CACHE STRING "Audio sink SBC codec quality preset")
set(PLUGIN_BLUETOOTHAUDIO_SOURCE_CONNECTOR "/tmp/bluetoothaudiosource" CACHE STRING "Audio source connector")
set(PLUGIN_BLUETOOTHAUDIO_SOURCE_CODECSBC_MAXBITPOOL "53" CACHE STRING "Audio source SBC codec maximum supported bitpool value")

add_library(${MODULE_NAME} SHARED
    BluetoothAudio.cpp
    BluetoothAudioSource.cpp
    BluetoothAudioSink.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_compile_options(${MODULE_NAME} 
    PRIVATE 
        -Wno-psabi
        -fdiagnostics-color=always)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ${NAMESPACE}Bluetooth::${NAMESPACE}Bluetooth
        ${NAMESPACE}BluetoothAudio::${NAMESPACE}BluetoothAudio)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)

write_config()
