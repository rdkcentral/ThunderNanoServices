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

option(INSTALL_COMPOSITOR_SERVER_TEST "Install compositor server test app" ON)

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(${NAMESPACE}Messaging CONFIG REQUIRED)
find_package(${NAMESPACE}LocalTracer CONFIG REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(${NAMESPACE}CompositorBufferType CONFIG REQUIRED)

add_executable(compositorserver main.cpp)

target_link_libraries(compositorserver
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}COM::${NAMESPACE}COM
        ${NAMESPACE}CompositorBufferType::${NAMESPACE}CompositorBufferType
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        CompileSettingsDebug::CompileSettingsDebug
)

if(INSTALL_COMPOSITOR_SERVER_TEST)
    install(TARGETS compositorserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/${NAMESPACE}Tests COMPONENT ${NAMESPACE}_Test)
endif()
