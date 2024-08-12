# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2021 Metrological
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

find_package(libdrm REQUIRED)

add_library(PixelBuffer STATIC PixelBuffer.cpp)

target_include_directories(PixelBuffer PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(PixelBuffer
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        libdrm::libdrm
)

add_library(PixelBuffer::PixelBuffer ALIAS PixelBuffer)


add_library(DmaBuffer STATIC DmaBuffer.cpp)

target_include_directories(DmaBuffer PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(DmaBuffer
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        libdrm::libdrm
        common::include
)

add_library(DmaBuffer::DmaBuffer ALIAS DmaBuffer)