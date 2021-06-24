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

set(TARGET ${PLATFORM_COMPOSITOR})

message(STATUS "Setting up ${TARGET} for RPI platfrom")

find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)

find_package(BCM_HOST QUIET)

if (BCM_HOST_FOUND)
    add_library(${TARGET}
        RPI.cpp)
else()
    find_package(GBM REQUIRED)
    find_package(LibDRM REQUIRED)

    if(NOT GBM_FOUND)
        message(FATAL_ERROR "The GBM library and its headers cannot be found.")
    endif()

    if(NOT LIBDRM_FOUND)
        message(FATAL_ERROR "The libDRM library and its headers cannot be found.")
    endif()

    add_library(${TARGET}
        ModeSet.cpp
        RPI.cpp)

    target_compile_definitions(${TARGET} PRIVATE VC6)

    target_link_libraries(${TARGET} 
        PRIVATE 
            libdrm::libdrm
            libgbm::libgbm
    )

endif()

target_link_libraries(${TARGET}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE)

install(TARGETS ${TARGET}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/Compositor
        )
