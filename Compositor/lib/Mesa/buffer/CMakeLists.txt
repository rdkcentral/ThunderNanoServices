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

project(CompositorBuffer)

cmake_minimum_required(VERSION 3.15)

if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/src/${COMPOSITOR_BUFFER_IMPLEMENTATION}")
    message(FATAL_ERROR "Missing a buffer implementation for ${COMPOSITOR_BUFFER_IMPLEMENTATION}")
endif()

message(STATUS "Buffer: ${COMPOSITOR_BUFFER_IMPLEMENTATION}")

add_subdirectory("src/${COMPOSITOR_BUFFER_IMPLEMENTATION}")
add_subdirectory(test)