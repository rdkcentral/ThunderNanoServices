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

project(AVS)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder REQUIRED)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(GStreamer REQUIRED)

#remove the -as-needed flag to ensure linking with AVSDevice and SmartScreen libs
string(REPLACE "-Wl,--as-needed" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-as-needed")

set(PLUGIN_AVS_NAME "AVS" CACHE STRING "The plugin name")
set(PLUGIN_AVS_STARTMODE "Deactivated" CACHE STRING "Automatically start plugin")
set(PLUGIN_AVS_MODE "Off" CACHE STRING "Controls if the plugin should run in its own process, in process or remote")
set(PLUGIN_AVS_PLATFORM "rpi3" CACHE STRING "Platform name (currently only rpi3)")
set(PLUGIN_AVS_DATA_PATH "/usr/share/Thunder/" CACHE STRING "Path were AVS data will be stored")
set(PLUGIN_AVS_ALEXA_CLIENT_CONFIG "${PLUGIN_AVS_DATA_PATH}/${PLUGIN_AVS_NAME}/AlexaClientSDKConfig.json" CACHE STRING "Path to AlexaClientSDKConfig")
set(PLUGIN_AVS_SMART_SCREEN_CONFIG "${PLUGIN_AVS_DATA_PATH}/${PLUGIN_AVS_NAME}/SmartScreenSDKConfig.json" CACHE STRING "Path to SmartScreenSDKConfig")
set(PLUGIN_AVS_LOG_LEVEL "DEBUG0" CACHE STRING "Default log level for the SDK")
set(PLUGIN_AVS_AUDIOSOURCE "PORTAUDIO" CACHE STRING "Callsign of plugin that provides voice input or PORTAUDIO")
set(PLUGIN_AVS_ENABLE_SMART_SCREEN_SUPPORT OFF CACHE BOOL "Compile in the Smart Screen support")
set(PLUGIN_AVS_ENABLE_SMART_SCREEN "false" CACHE STRING "Enable the Smart Screen support in the runtime (true/false)")
set(PLUGIN_AVS_ENABLE_KWD_SUPPORT ON CACHE BOOL "Compile in the Pryon Keyword Detection engine")
set(PLUGIN_AVS_ENABLE_KWD "false" CACHE STRING "Enable the Pryon Keyword Detection engine in the runtime (true/false)")
set(PLUGIN_AVS_KWD_MODELS_PATH "${PLUGIN_AVS_DATA_PATH}/${PLUGIN_AVS_NAME}/models" CACHE STRING "Path to KWD input directory")

if(GSTREAMER_FOUND)
    add_definitions(-DGSTREAMER_MEDIA_PLAYER)
endif()

add_definitions(-DRAPIDJSON_HAS_STDSTRING)

add_library(${MODULE_NAME} SHARED
    Module.cpp
    AVS.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        LINK_WHAT_YOU_USE TRUE
        CXX_STANDARD_REQUIRED YES)

add_subdirectory("Impl/AVSDevice/")

if(PLUGIN_AVS_ENABLE_SMART_SCREEN_SUPPORT)
    add_subdirectory("Impl/SmartScreen/")
    target_link_libraries(${MODULE_NAME} PRIVATE SmartScreen)
endif()

add_subdirectory("Integration")

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        AVSDevice)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)

# TODO: remove me ;)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")

write_config()
