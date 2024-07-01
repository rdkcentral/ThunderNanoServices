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

project(DIALServer)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)


option(PLUGIN_DIALSERVER_ENABLE_YOUTUBE "Enable YouTube support for DIAL server" OFF)
option(PLUGIN_DIALSERVER_ENABLE_NETFLIX "Enable Netflix support for DIAL server" OFF)
option(PLUGIN_DIALSERVER_ENABLE_AMAZON_PRIME "Enable Amazon Prime support for DIAL server" OFF)

set(PLUGIN_DIALSERVER_STARTMODE "Activated" CACHE STRING "Automatically start DIALServer plugin")

set(PLUGIN_DIALSERVER_YOUTUBE_MODE "passive" CACHE STRING "How the DIAL server should process incomming requests from Youtube (passive/active), leave empty to disable")
set(PLUGIN_DIALSERVER_NETFLIX_MODE "passive" CACHE STRING "How the DIAL server should process incomming requests from Netflix (passive/active), leave empty to disable")
set(PLUGIN_DIALSERVER_AMAZON_PRIME_MODE "passive" CACHE STRING "How the DIAL server should process incomming requests from Amazon Prime (passive/active), leave empty to disable")

set(PLUGIN_DIALSERVER_YOUTUBE_RUNTIMECHANGE true CACHE STRING "Enable runtime change for YouTube")

set(PLUGIN_DIALSERVER_SWITCHBOARD_CALLSIGN "" CACHE STRING "Callsign of the SwitchBoard plugin if used, leave empty to disable")
set(PLUGIN_DIALSERVER_WEBSERVER_CALLSIGN "" CACHE STRING "Callsign of the WebServer plugin if used, leave empty to disable")
set(PLUGIN_DIALSERVER_NETFLIX_CALLSIGN "" CACHE STRING "Callsign of the Netflix plugin if used in active mode, leave empty to disable")
set(PLUGIN_DIALSERVER_YOUTUBE_CALLSIGN "" CACHE STRING "Callsign of the Youtube plugin if used in active mode, leave empty to disable")
set(PLUGIN_DIALSERVER_AMAZON_PRIME_CALLSIGN "" CACHE STRING "Callsign of the Youtube plugin if used in active mode, leave empty to disable")

set(PLUGIN_DIALSERVER_NETFLIX_HIDE "false" CACHE STRING "Whether target service can be hidden")

set(PLUGIN_DIALSERVER_NAME "${MODULE_NAME}" CACHE STRING "DIAL server name")
set(PLUGIN_DIALSERVER_MODEL "Generic Platform" CACHE STRING "Model of the device")
set(PLUGIN_DIALSERVER_MANUFACTURER "Metrological" CACHE STRING "Manufacturer of device")
set(PLUGIN_DIALSERVER_DESCRIPTION "Metrological DIAL reference server." CACHE STRING "Description")

add_library(${MODULE_NAME} SHARED
    DIALServer.cpp
    DIALServerJsonRpc.cpp
    Module.cpp)

if(PLUGIN_DIALSERVER_ENABLE_YOUTUBE AND "${PLUGIN_DIALSERVER_YOUTUBE_MODE}" STREQUAL "active")
    target_sources(${MODULE_NAME}
        PRIVATE
            YouTube.cpp)
endif()

if(PLUGIN_DIALSERVER_ENABLE_AMAZON_PRIME AND "${PLUGIN_DIALSERVER_AMAZON_PRIME_MODE}" STREQUAL "active")
target_sources(${MODULE_NAME}
        PRIVATE
            AmazonPrime.cpp)
endif()

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)

write_config()
