# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2024 Metrological
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

project(ShowcaseInterfaces)

find_package(Thunder)
find_package(${NAMESPACE}Core REQUIRED)
find_package(CompileSettingsDebug REQUIRED)

find_package(ProxyStubGenerator REQUIRED)
find_package(JsonGenerator REQUIRED)

file(GLOB INTERFACE_HEADERS "I*.h")

if(NOT GENERATOR_SEARCH_PATH)
    set(GENERATOR_SEARCH_PATH ${CMAKE_SYSROOT}${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE})
endif()

JsonGenerator(CODE INPUT ${INTERFACE_HEADERS} VERBOSE INCLUDE_PATH ${GENERATOR_SEARCH_PATH})

file(GLOB JSON_ENUM_SOURCES "JsonEnum*.cpp")
file(GLOB JSON_HEADERS "J*.h")

set(SHOWCASEINTERFACES_DEFINITIONS ${MODULE_NAME}Definitions CACHE INTERNAL "")

add_library(${SHOWCASEINTERFACES_DEFINITIONS} SHARED
    ${JSON_ENUM_SOURCES}
    Module.cpp)

target_include_directories(${SHOWCASEINTERFACES_DEFINITIONS} SYSTEM
    PRIVATE
        "..")

target_link_libraries(${SHOWCASEINTERFACES_DEFINITIONS}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        CompileSettingsDebug::CompileSettingsDebug)

install(FILES ${JSON_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/interfaces/json COMPONENT ${NAMESPACE}_Development)
install(TARGETS ${SHOWCASEINTERFACES_DEFINITIONS} DESTINATION ${CMAKE_INSTALL_LIBDIR}/ COMPONENT ${NAMESPACE}_Runtime)