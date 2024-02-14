# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2023 Metrological
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
find_package(wayland-client REQUIRED)
find_package(wayland-protocols REQUIRED)

WaylandGenerator(
    HEADERS WaylandHeaders
    SOURCES WaylandSources
    DEFENITIONS     
        xdg-shell
        presentation-time
        xdg-decoration-unstable-v1
        linux-dmabuf-unstable-v1
        pointer-gestures-unstable-v1
        relative-pointer-unstable-v1
        ${CMAKE_CURRENT_LIST_DIR}/protocols/xdg-activation-v1.xml
        ${CMAKE_CURRENT_LIST_DIR}/protocols/drm.xml
)

add_library(WaylandBackend STATIC Wayland.cpp Input.cpp Output.cpp ${WaylandSources})

target_include_directories(WaylandBackend PRIVATE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(WaylandBackend
    PUBLIC
        common::include
        common::transformation
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        common::buffer
        common::drm
        gbm::gbm
        wayland-client::wayland-client
        wayland-protocols::wayland-protocols
)

set_target_properties(WaylandBackend PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        POSITION_INDEPENDENT_CODE ON
)

add_library(common::backend ALIAS WaylandBackend)
