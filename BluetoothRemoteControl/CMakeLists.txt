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

set(PLUGIN_NAME BluetoothRemoteControl)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Bluetooth REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

set(PLUGIN_BLUETOOTHREMOTECONTROL_SUPPORT_ADPCM_HQ true CACHE BOOL "Support adpcm-hq audio profile")
set(PLUGIN_BLUETOOTHREMOTECONTROL_SUPPORT_PCM true CACHE BOOL "Support pcm audio profile")

add_library(${MODULE_NAME} SHARED
    BluetoothRemoteControl.cpp
    BluetoothRemoteControlJsonRpc.cpp
    Administrator.cpp
    T4HDecoders.cpp
    4ModDecoders.cpp
    HID.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ${NAMESPACE}Bluetooth::${NAMESPACE}Bluetooth)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

install(
	DIRECTORY keymaps/
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/${PLUGIN_NAME}
	FILES_MATCHING PATTERN "*.json")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")

write_config(${PLUGIN_NAME})
