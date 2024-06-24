# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2022 Metrological
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

cmake_minimum_required(VERSION 3.15)

project(YinInterfaces)

find_package(Thunder)
find_package(${NAMESPACE}Core REQUIRED)
find_package(CompileSettingsDebug REQUIRED)

find_package(ProxyStubGenerator REQUIRED)
find_package(JsonGenerator REQUIRED)

file(GLOB INTERFACE_HEADERS "I*.h")

ProxyStubGenerator(INPUT ${INTERFACE_HEADERS})
JsonGenerator(CODE INPUT ${INTERFACE_HEADERS})

file(GLOB PROXY_STUB_SOURCES "ProxyStubs*.cpp")
file(GLOB JSON_ENUM_SOURCES "JsonEnum*.cpp")
file(GLOB JSON_HEADERS "J*.h")

set(PLUGIN_YIN_MARSHALLING ${MODULE_NAME}Marshalling CACHE INTERNAL "")
set(PLUGIN_YIN_DEFINITIONS ${MODULE_NAME}Definitions CACHE INTERNAL "")

add_library(${PLUGIN_YIN_MARSHALLING} SHARED
    ${PROXY_STUB_SOURCES}
    Module.cpp)

target_link_libraries(${PLUGIN_YIN_MARSHALLING}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        CompileSettingsDebug::CompileSettingsDebug)

add_library(${PLUGIN_YIN_DEFINITIONS} SHARED
    ${JSON_ENUM_SOURCES}
    Module.cpp)

target_link_libraries(${PLUGIN_YIN_DEFINITIONS}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        CompileSettingsDebug::CompileSettingsDebug)

install(TARGETS ${PLUGIN_YIN_MARSHALLING} DESTINATION ../${EXAMPLE_DYNAMICLOADING_TARGET}/usr/${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/proxystubs COMPONENT ${NAMESPACE}_Runtime)
install(TARGETS ${PLUGIN_YIN_DEFINITIONS} DESTINATION ../${EXAMPLE_DYNAMICLOADING_TARGET}/usr/${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Runtime)
