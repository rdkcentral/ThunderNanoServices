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

set(PLUGIN_NAME DIALServer)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

option(PLUGIN_DIALSERVER_ENABLE_YOUTUBE "Enable YouTube support for DIAL server" OFF)
option(PLUGIN_DIALSERVER_ENABLE_NETFLIX "Enable Netflix support for DIAL server" OFF)

set(PLUGIN_DIALSERVER_YOUTUBE_MODE "passive" CACHE STRING "How the DIAL server should process incomming requests from Youtube (passive/active), leave empty to disable")
set(PLUGIN_DIALSERVER_NETFLIX_MODE "passive" CACHE STRING "How the DIAL server should process incomming requests from Netflix (passive/active), leave empty to disable")

set(PLUGIN_DIALSERVER_SWITCHBOARD_CALLSIGN "" CACHE STRING "Callsign of the SwitchBoard plugin if used, leave empty to disable")
set(PLUGIN_DIALSERVER_WEBSERVER_CALLSIGN "" CACHE STRING "Callsign of the WebServer plugin if used, leave empty to disable")
set(PLUGIN_DIALSERVER_NETFLIX_CALLSIGN "" CACHE STRING "Callsign of the Netflix plugin if used in active mode, leave empty to disable")
set(PLUGIN_DIALSERVER_YOUTUBE_CALLSIGN "" CACHE STRING "Callsign of the Youtube plugin if used in active mode, leave empty to disable")

set(PLUGIN_DIALSERVER_NAME "${MODULE_NAME}" CACHE STRING "DIAL server name")
set(PLUGIN_DIALSERVER_MODEL "Generic Platform" CACHE STRING "Model of the device")
set(PLUGIN_DIALSERVER_MANUFACTURER "Metrological" CACHE STRING "Manufacturer of device")
set(PLUGIN_DIALSERVER_DESCRIPTION "Metrological DIAL reference server." CACHE STRING "Description")

add_library(${MODULE_NAME} SHARED
    DIALServer.cpp
    DIALServerJsonRpc.cpp
    Module.cpp)

if(PLUGIN_DIALSERVER_ENABLE_YOUTUBE)
    target_sources(${MODULE_NAME}
        PRIVATE
            YouTube.cpp)
endif()

if(PLUGIN_DIALSERVER_ENABLE_NETFLIX)
    target_sources(${MODULE_NAME}
        PRIVATE
            Netflix.cpp)
endif()

if (NETFLIX_VERSION_5_1)
    target_compile_definitions(${MODULE_NAME} 
        PRIVATE 
            NETFLIX_VERSION_5_1=ON)
endif()
if(NETFLIX_VERSION_5_2)
    target_compile_definitions(${MODULE_NAME}
    PRIVATE
        NETFLIX_VERSION_5_2=ON)
endif()

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

install(TARGETS ${MODULE_NAME}
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
