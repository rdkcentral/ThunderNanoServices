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

find_package(gbm REQUIRED)

add_library(CommonBufferGBM STATIC GBM.cpp)

option(ENABLE_DUMP_ON_COMPLETED "Dump a png appon completing the buffer" OFF)

target_link_libraries(CommonBufferGBM 
    PUBLIC
        common::include
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        gbm::gbm
        common::drm
)

if(ENABLE_DUMP_ON_COMPLETED)
    find_package(PNG REQUIRED)
    target_compile_definitions(CommonBufferGBM PRIVATE ENABLE_DUMP=1)
    target_link_libraries(CommonBufferGBM PRIVATE PNG::PNG)
endif()

target_compile_definitions(CommonBufferGBM
    PRIVATE
        HAVE_GBM_MODIFIERS=1
        HAS_GBM_BO_GET_FD_FOR_PLANE=1
)

set_target_properties(CommonBufferGBM PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        POSITION_INDEPENDENT_CODE ON
)

add_library(common::buffer ALIAS CommonBufferGBM)
