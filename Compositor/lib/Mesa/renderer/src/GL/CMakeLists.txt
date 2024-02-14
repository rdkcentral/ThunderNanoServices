# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2022 Metrological
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

find_package(EGL REQUIRED)
find_package(GLESv2 REQUIRED)
find_package(PNG REQUIRED)

find_package(gbm REQUIRED)

add_subdirectory(shaders)

add_library(CommonRendererGLES STATIC GLES2.cpp EGL.cpp)

target_link_libraries(CommonRendererGLES
    PUBLIC
        common::include
        common::transformation
    PRIVATE
        common::drm
        gl::shaders
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        EGL::EGL
        GLESv2::GLESv2
        libdrm::libdrm
        gbm::gbm
        PNG::PNG
)

set_target_properties(CommonRendererGLES PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        POSITION_INDEPENDENT_CODE ON
)

target_compile_definitions(CommonRendererGLES
    PUBLIC
        EGL_NO_X11
        __DEBUG__
    )

add_library(common::renderer ALIAS CommonRendererGLES)

add_subdirectory(test)
