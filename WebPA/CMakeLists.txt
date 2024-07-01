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

project(WebPA)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

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
find_package(WDMP-C REQUIRED)
find_package(ParodusService REQUIRED)

set(PLUGIN_WEBPA_INTERFACE "eth0" CACHE STRING "Interface name to be used for establishing connection")
set(PLUGIN_WEBPA_PINGWAITTIME "60" CACHE STRING "Wait time out for Ping in seconds")
set(PLUGIN_WEBPA_WEBPAURL "http://192.168.1.1" CACHE STRING "URL of WEBPA Server")
set(PLUGIN_WEBPA_PARODUSLOCALURL "tcp://127.0.0.1:6666" CACHE STRING "URL of Parodus Local Service")
set(PLUGIN_WEBPA_PARTENRID "Metrological" CACHE STRING "Parter ID")
set(PLUGIN_WEBPA_BACKOFFMAX "9" CACHE STRING "Max retry sleep for WebPA connection")
set(PLUGIN_WEBPA_CERTPATH "/etc/ssl/certs/ca-certificates.crt" CACHE STRING "SSL Certificate Path of Parodus Service")
set(PLUGIN_WEBPA_CCSP_CLIENTURL "tcp://127.0.0.1:6667" CACHE STRING "URL of CCSP Client to communicate with Service")
set(PLUGIN_WEBPA_CCSP_CONFIGFILE "/etc/webpa_cfg.json" CACHE STRING "Config File for CCSP Client")
set(PLUGIN_WEBPA_CCSP_DATAFILE "" CACHE STRING "Device properties file for CCSP client")
set(PLUGIN_WEBPA_GENERICCLIENTURL "tcp://127.0.0.1:6667" CACHE STRING "URL of Generic Client to communicate with Service")
set(PLUGIN_WEBPA_GENERICCLIENT_MAXRETRY "1" CACHE STRING "Number of retries to establish a connection with parodus service")
set(PLUGIN_WEBPA_DATAMODELFILE "/usr/share/Thunder/WebPA/data-model.xml" CACHE STRING "Data Model File for Generic Adapter")
set(PLUGIN_WEBPA_NOTIFYCONFIGFILE "/usr/share/Thunder/WebPA/notify_webpa_cfg.json" CACHE STRING "Notifier configuration file for Generic Adapter")

add_library(${MODULE_NAME} SHARED
    WebPA.cpp
    Module.cpp
    WebPAImplementation.cpp
)

target_compile_definitions(${MODULE_NAME} PRIVATE ${PLUGIN_DEFINITIONS})
target_include_directories(${MODULE_NAME} PRIVATE ${PLUGIN_INCLUDE_DIRS})

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ParodusService::ParodusService
        WDMP-C::WDMP-C
)

string(TOLOWER ${NAMESPACE} STORAGE_DIRECTORY)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)

set(PLUGIN_WEBPA_STARTMODE "Deactivated" CACHE STRING "Automatically start WebPA plugin")
set(PLUGIN_WEBPA_MODE "Local" CACHE STRING "Controls if the plugin should run in its own process, in process or remote.")

write_config()

add_subdirectory(Clients)
