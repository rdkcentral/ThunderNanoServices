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

project(SimpleCOMRPCInterface)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

find_package(${NAMESPACE}Core)
find_package(${NAMESPACE}COM)
find_package(CompileSettingsDebug CONFIG REQUIRED)

find_package(ProxyStubGenerator REQUIRED)

if(NOT GENERATOR_SEARCH_PATH)
    set(GENERATOR_SEARCH_PATH ${CMAKE_SYSROOT}${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE})
endif()

ProxyStubGenerator(INPUT "${CMAKE_CURRENT_SOURCE_DIR}" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated" INCLUDE_PATH ${GENERATOR_SEARCH_PATH})

file(GLOB INTERFACES_HEADERS I*.h)
list(APPEND INTERFACES_HEADERS Module.h)

file(GLOB PROXY_STUB_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs*.cpp")

add_library(${MODULE_NAME} SHARED
        ${PROXY_STUB_SOURCES}        
        Module.cpp
        )

target_link_libraries(${MODULE_NAME}
        PRIVATE
                ${NAMESPACE}COM::${NAMESPACE}COM
        )

target_link_libraries(${MODULE_NAME}
        PRIVATE
          CompileSettingsDebug::CompileSettingsDebug
        )

set_target_properties(${TargetMarshalling} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        )

install(
        MODULE_NAMES ${MODULE_NAME}  EXPORT ${MODULE_NAME}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${NAMESPACE_LIB}/proxystubs COMPONENT ${NAMESPACE}_Development
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${NAMESPACE_LIB}/proxystubs COMPONENT ${NAMESPACE}_Runtime
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR}/${NAMESPACE_LIB}/proxystubs COMPONENT ${NAMESPACE}_Runtime
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/proxystubs COMPONENT ${NAMESPACE}_Development
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/proxystubs  
)

InstallCMakeConfig(MODULE_NAMES ${MODULE_NAME})
