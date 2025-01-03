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
find_package(${NAMESPACE}Messaging CONFIG REQUIRED)
find_package(${NAMESPACE}Definitions CONFIG REQUIRED)

find_package(gbm REQUIRED)

set(ENABLE_ATOMIC OFF CACHE BOOL "Enable the atomic API for KMS/DRM Backend")

add_library(CommonBackendDRM STATIC DRM.cpp)

message(STATUS "   Atomic: ${ENABLE_ATOMIC}")

if(ENABLE_ATOMIC)
    target_compile_definitions(CommonBackendDRM PRIVATE USE_ATOMIC)
endif()

target_link_libraries(CommonBackendDRM
    PUBLIC
        common::include
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        common::buffer
        common::drm
        gbm::gbm
)

set_target_properties(CommonBackendDRM PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        POSITION_INDEPENDENT_CODE ON
)

add_library(common::backend ALIAS CommonBackendDRM)
