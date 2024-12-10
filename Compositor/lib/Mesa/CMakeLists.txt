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

message(STATUS "Setting up ${PLATFORM_COMPOSITOR} for Mesa platfrom")

find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}COM REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(ClientCompositorBufferType REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

find_package(gbm REQUIRED)
find_package(libdrm REQUIRED)
find_package(EGL REQUIRED)
find_package(GLESv2 REQUIRED)

set(COMPOSITOR_BUFFER_IMPLEMENTATION  "GBM" CACHE STRING "Defines which buffer implementation is used")
set(COMPOSITOR_RENDERER_IMPLEMENTATION "GL" CACHE STRING "Defines which renderer implementation is used")
set(COMPOSITOR_BACKEND_IMPLEMENTATION  "DRM" CACHE STRING "Defines which backend implementation is used")

add_subdirectory(include)
add_subdirectory(drm)
add_subdirectory(buffer)
add_subdirectory(backend)
add_subdirectory(renderer)

add_library(${PLATFORM_COMPOSITOR} SHARED Compositor.cpp)

target_link_libraries(${PLATFORM_COMPOSITOR}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}COM::${NAMESPACE}COM
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ClientCompositorBufferType::ClientCompositorBufferType
        CompileSettingsDebug::CompileSettingsDebug
        common::include
        common::buffer
        common::backend
        common::transformation
        common::renderer
        common::drm
)

set_target_properties(${PLATFORM_COMPOSITOR} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        POSITION_INDEPENDENT_CODE ON
)

install(TARGETS ${PLATFORM_COMPOSITOR}
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${NAMESPACE}/Compositor COMPONENT ${NAMESPACE}_Runtime
)

add_library(compositor::platform ALIAS ${PLATFORM_COMPOSITOR})

# add_subdirectory(test)
