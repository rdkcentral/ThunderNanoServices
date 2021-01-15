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

set(PLUGIN_NAME Power)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(PLUGIN_POWER_AUTOSTART true CACHE STRING true)
set(PLUGIN_POWER_GPIOPIN "" CACHE STRING "GPIO pin number")
set(PLUGIN_POWER_GPIOTYPE "" CACHE STRING "GPIO type")
# PLUGIN_POWER_POWER_KEY is for setting the keycode of the key that needs to be configured as power button.
# default value `116` corresponds to KEY_POWER. This value comes from input-event-codes.h
# https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
# If this feature is not needed, set this value to 0
set(PLUGIN_POWER_POWER_KEY 116 CACHE STRING "Key Code for power key. To disable set this to 0")
set(PLUGIN_POWER_CONTROL_CLIENTS true CACHE STRING "Control plugins with IStateControl interface")
set(PLUGIN_POWER_IMPLEMENTATION "Linux" CACHE STRING "Implementation to be selected for the PowerPlugin, if it could not be autodetected.")

option(PLUGIN_POWER_MFRPERSIST_STATE "Enable device to boot to last power state on power loss reboot. [ON, OFF]." OFF)

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
    Power.cpp
    PowerJsonRpc
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

target_include_directories( ${MODULE_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

find_package(BcmPowerManager QUIET)
find_package(MFRPwrMgrLibs QUIET)

if(${PLUGIN_POWER_MFRPERSIST_STATE} STREQUAL ON AND MFRPWRMGRLIBS_FOUND)
    message("Enabling Persistent power state feature...")
    target_sources(${MODULE_NAME} PRIVATE
        PowerImplementation/${PLUGIN_POWER_IMPLEMENTATION}/MFRPersistPower.cpp)
    target_link_libraries(${MODULE_NAME} PRIVATE pwrmgrlibs::pwrmgrlibs)
else()
    target_sources(${MODULE_NAME} PRIVATE
        PowerImplementation/Stub/MFRPersistPowerStub.cpp)
endif()

if(BCM_PM_FOUND)
    find_package(NEXUS REQUIRED)
    find_package(NXCLIENT REQUIRED)
    if (NXCLIENT_FOUND)
        message("Configuring for Nexus....")
        set(POWER_LIBS -Wl,--whole-archive ${BCM_PM_LIBRARIES} -Wl,--no-whole-archive)
        if (POWER_IMPLEMENTATION_PATH)
            target_sources(${MODULE_NAME}
                PRIVATE
                    ${POWER_IMPLEMENTATION_PATH}/PowerImplementation.cpp)
        else()
            include(GetExternalCode)
            set(POWER_IMPLEMENTATION_VERSION "master" CACHE STRING "Power implementation version")
            set(POWER_IMPLEMENTATION_REPOSITORY "https://code.rdkcentral.com/r/soc/broadcom/components/rdkcentral/thundernanoservices/Power" CACHE STRING "Power implementation repository")
            GetExternalCode(
                GIT_REPOSITORY ${POWER_IMPLEMENTATION_REPOSITORY}
                GIT_TAG ${POWER_IMPLEMENTATION_VERSION}
                SOURCE_DIR "PowerImplementation/Broadcom"
            )
            target_sources(${MODULE_NAME}
                PRIVATE
                    PowerImplementation/Broadcom/PowerImplementation.cpp)
        endif()
        target_link_libraries(${MODULE_NAME}
            PRIVATE
                NEXUS::NEXUS
                NXCLIENT::NXCLIENT
                ${POWER_LIBS})
    else()
        message("Error: missing Nexus libraries....")
    endif()
else()
    message("Configuring for ${PLUGIN_POWER_IMPLEMENTATION}....")
    target_sources(${MODULE_NAME} PRIVATE
        PowerImplementation/${PLUGIN_POWER_IMPLEMENTATION}/PowerImplementation.cpp)
endif()

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

install(TARGETS ${MODULE_NAME}
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
