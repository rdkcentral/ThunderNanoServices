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

project(Snapshot)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

set(PLUGIN_SNAPSHOT_STARTMODE "Activated" CACHE STRING "Automatically start Snapshot plugin")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(BCM_HOST QUIET)
find_package(NEXUS QUIET)
find_package(NXCLIENT QUIET)

add_library(${MODULE_NAME} SHARED
        Module.cpp
        Snapshot.cpp)

target_link_libraries(${MODULE_NAME} 
    PRIVATE 
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        PNG::PNG)

set_target_properties(${MODULE_NAME}
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

if (NXCLIENT_FOUND AND NEXUS_FOUND)
    if (SNAPSHOT_IMPLEMENTATION_PATH)
        target_sources(${MODULE_NAME}
            PRIVATE
                ${SNAPSHOT_IMPLEMENTATION_PATH}/NexusCapture.cpp)
    else()
        include(GetExternalCode)
        set(SNAPSHOT_IMPLEMENTATION_VERSION "master" CACHE STRING "Snapshot implementation version: commit_id_or_tag_or_branch_name")
        set(SNAPSHOT_IMPLEMENTATION_REPOSITORY "https://code.rdkcentral.com/r/soc/broadcom/components/rdkcentral/thundernanoservices/Snapshot" CACHE STRING "Snapshot implementation repository")
        GetExternalCode(
            GIT_REPOSITORY ${SNAPSHOT_IMPLEMENTATION_REPOSITORY}
            GIT_VERSION ${SNAPSHOT_IMPLEMENTATION_VERSION}
            SOURCE_DIR "Nexus"
        )
        target_sources(${MODULE_NAME}
            PRIVATE
                Nexus/NexusCapture.cpp)
    endif()
    target_link_libraries(${MODULE_NAME} 
        PRIVATE 
            NEXUS::NEXUS 
            NXCLIENT::NXCLIENT) 
elseif (BCM_HOST_FOUND)
    target_link_libraries(${MODULE_NAME} 
        PRIVATE 
            BCM_HOST::BCM_HOST)
    target_sources(${MODULE_NAME} 
        PRIVATE 
            Device/Dispmanx.cpp)
else ()
    message(FATAL_ERROR "There is no graphic backend for Snapshot plugin")
endif ()

install(TARGETS ${MODULE_NAME} 
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Runtime)

write_config()
