# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 Metrological
#
# Licensed under the Apache License, Version 2.0 (the License);
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

project(SimpleCOMRPCServer)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

find_package(${NAMESPACE}COM REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_executable(${MODULE_NAME} SimpleCOMRPCServer.cpp)

set_target_properties(COMRPCClient PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )     

target_link_libraries(${MODULE_NAME}
        PRIVATE
        ${NAMESPACE}COM::${NAMESPACE}COM
        CompileSettingsDebug::CompileSettingsDebug
    )

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime)
