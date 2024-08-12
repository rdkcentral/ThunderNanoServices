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

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}COM REQUIRED)
find_package(${NAMESPACE}WebSocket REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_executable(StoreTest StoreTest.cpp)

set_target_properties(StoreTest PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )     

target_link_libraries(StoreTest 
        PRIVATE
        ${NAMESPACE}COM::${NAMESPACE}COM
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
	${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
        CompileSettingsDebug::CompileSettingsDebug
    )

install(TARGETS StoreTest DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Test)
