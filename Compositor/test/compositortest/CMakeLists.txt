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

option(INSTALL_COMPOSITOR_CLIENT_TEST "Build compositor client test app" OFF)

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)

find_package(GLESv2 REQUIRED)
find_package(EGL REQUIRED)
find_package(PNG REQUIRED)

find_package(ClientCompositor CONFIG REQUIRED)

add_executable(CompositorTest Test.cpp)

set_target_properties(CompositorTest PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )     

target_link_libraries(CompositorTest
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        CompileSettingsDebug::CompileSettingsDebug
        ClientCompositor::ClientCompositor
        PNG::PNG       
         ${EGL_LIBRARIES}
         ${GLESV2_LIBRARIES}
        )

target_include_directories(CompositorTest
    PRIVATE
      ${EGL_INCLUDE_DIRS}
      ${GLESV2_INCLUDE_DIRS})

target_compile_definitions(CompositorTest
    PRIVATE
        ${EGL_DEFINITIONS}
        ${GLESV2_DEFINITIONS})

if(INSTALL_COMPOSITOR_CLIENT_TEST)
    install(TARGETS CompositorTest DESTINATION ${CMAKE_INSTALL_BINDIR}/${NAMESPACE}Tests COMPONENT ${NAMESPACE}_Test)
endif(INSTALL_COMPOSITOR_CLIENT_TEST)
