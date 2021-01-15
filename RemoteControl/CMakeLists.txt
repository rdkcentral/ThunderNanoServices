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

set(PLUGIN_NAME RemoteControl)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(PLUGIN_REMOTECONTROL_RELEASE_TIMEOUT 30000 CACHE STRING "Remote control release timeout")
set(PLUGIN_REMOTECONTROL_PASSON false CACHE STRING "Enable keys pass-through on default producer")

option(PLUGIN_REMOTECONTROL_RFCE "Enable RF4CE functionality." ON)

set(PLUGIN_REMOTECONTROL_RFCE_REMOTE_ID "GPSTB" CACHE STRING "User string, used for greenpeak")
set(PLUGIN_REMOTECONTROL_RFCE_MODULE "/lib/modules/misc/gpK5.ko" CACHE STRING "path to kernel module")
set(PLUGIN_REMOTECONTROL_RFCE_NODE_ID 249 CACHE STRING "Node ID for the device to be created")
set(PLUGIN_REMOTECONTROL_RFCE_AUTOPAIRING false CACHE BOOL "Enable AutoPairing feature for RF4CE producer")
set(PLUGIN_REMOTECONTROL_RFCE_NVM_LUT "DEFAULT" CACHE STRING "The NVM LUT to be used (see GreenPeakNvmLut.h)")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(NEXUS QUIET)
find_package(NXCLIENT QUIET)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
    Module.cpp 
    RemoteControl.cpp 
    RemoteAdministrator.cpp
    RemoteControlJsonRpc.cpp)

target_link_libraries(${MODULE_NAME} 
    PRIVATE 
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

set_target_properties(${MODULE_NAME} PROPERTIES 
    CXX_STANDARD 11
	CXX_STANDARD_REQUIRED YES)

if (NXCLIENT_FOUND AND NEXUS_FOUND)
    if (PLUGIN_REMOTECONTROL_IR)
        message(STATUS "Including IR support")
        target_compile_definitions(${MODULE_NAME} 
            PRIVATE 
                IR_ENABLED)
        if (REMOTECONTROL_IMPLEMENTATION_PATH)
            target_sources(${MODULE_NAME}
                PRIVATE
                    ${REMOTECONTROL_IMPLEMENTATION_PATH}/IRRemote.cpp)
        else()
            include(GetExternalCode)
            set(REMOTECONTROL_IMPLEMENTATION_VERSION "master" CACHE STRING "RemoteControl implementation version")
            set(REMOTECONTROL_IMPLEMENTATION_REPOSITORY "https://code.rdkcentral.com/r/soc/broadcom/components/rdkcentral/thundernanoservices/RemoteControl" CACHE STRING "RemoteControl implementation repository")
            GetExternalCode(
                GIT_REPOSITORY ${REMOTECONTROL_IMPLEMENTATION_REPOSITORY}
                GIT_TAG ${REMOTECONTROL_IMPLEMENTATION_VERSION}
                SOURCE_DIR "Nexus"
            )
            target_sources(${MODULE_NAME}
                PRIVATE
                    Nexus/IRRemote.cpp)
        endif()
        target_link_libraries(${MODULE_NAME} 
            PRIVATE 
                NEXUS::NEXUS 
                NXCLIENT::NXCLIENT)
    endif()
endif()

if(PLUGIN_REMOTECONTROL_RFCE)
    find_package(RF4CE QUIET)
    if (RF4CE_FOUND)
        message(STATUS "Including RF4CE support")
        add_definitions(${RF4CE_DEFINITIONS})
        target_compile_definitions(${MODULE_NAME} 
            PRIVATE 
                GP_SCHED_EXTERNAL_MAIN
                GP_NVM_LUT_${PLUGIN_REMOTECONTROL_RFCE_NVM_LUT})
        include_directories(${RF4CE_INCLUDE_DIRS})
        target_sources(${MODULE_NAME} 
            PRIVATE 
                GreenPeak.cpp)
        target_link_libraries(${MODULE_NAME} 
            PRIVATE 
                ${RF4CE_LIBRARIES})
    endif()
endif()

if (PLUGIN_REMOTECONTROL_DEVINPUT)
    message(STATUS "Including dev input support")
    target_sources(${MODULE_NAME} 
        PRIVATE 
            LinuxDevice.cpp)
    target_link_libraries(${MODULE_NAME} 
        PRIVATE 
            udev)
endif()

install(TARGETS ${MODULE_NAME} 
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

install(
	FILES linux-windows.json
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/RemoteControl
	COMPONENT ${MODULE_NAME})

install(
	FILES web-remote.json
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/RemoteControl
	COMPONENT ${MODULE_NAME})

write_config(${PLUGIN_NAME})
