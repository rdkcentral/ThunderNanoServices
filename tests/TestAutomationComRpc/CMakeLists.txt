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

project(TestAutomationComRpc)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

set(PLUGIN_TESTAUTOMATIONCOMRPC_STARTMODE "Activated" CACHE STRING "Automatically start TestAutomationComRpc plugin")
set(PLUGIN_TESTAUTOMATIONCOMRPC_RESUMED "true" CACHE STRING "Set TestAutomationComRpc resume state")
set(PLUGIN_TESTAUTOMATIONCOMRPC_MODE "Local" CACHE STRING "Controls if the TestAutomationComRpc should run in its own process, in process or remote.")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Tracing REQUIRED)

find_package(CompileSettingsDebug CONFIG REQUIRED)

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

add_library(${MODULE_NAME} SHARED
        Module.cpp
        TestAutomationComRpc.cpp
        TestAutomationComRpcImplementation.cpp)

target_link_libraries(${MODULE_NAME} 
    PRIVATE 
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Tracing::${NAMESPACE}Tracing
    PUBLIC
        ${EXTRA_LIBS}
    )

set_target_properties(${MODULE_NAME}
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

install(TARGETS ${MODULE_NAME} 
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Test)
    
write_config()
