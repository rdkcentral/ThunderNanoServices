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

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(${NAMESPACE}Tracing CONFIG REQUIRED)

find_package(gbm REQUIRED)
find_package(libdrm REQUIRED)

set(ENABLE_ATOMIC OFF CACHE BOOL "Enable the atomic API for KMS/DRM Backend")

add_library(DRMBackendImplentation OBJECT DRM.cpp)

if(ENABLE_ATOMIC)
target_sources(DRMBackendImplentation PRIVATE Atomic.cpp)
else()
target_sources(DRMBackendImplentation PRIVATE Legacy.cpp)
endif()


set_target_properties(DRMBackendImplentation PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_link_libraries(DRMBackendImplentation
    PUBLIC
        common::include
        common::buffer
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Tracing::${NAMESPACE}Tracing
        libdrm::libdrm
        gbm::gbm
        udev
)

target_include_directories(DRMBackendImplentation
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    )

add_library(common::backend ALIAS DRMBackendImplentation)
