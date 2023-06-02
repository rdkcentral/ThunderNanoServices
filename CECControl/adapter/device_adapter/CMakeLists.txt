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

find_package(Threads REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Messaging REQUIRED)

message(STATUS "Building ${CEC_DEVICE_ADAPTER_LIBRARY_NAME} for ${CEC_DEVICE_ADAPTER_IMPLEMENTATION}")

add_library(${CEC_DEVICE_ADAPTER_LIBRARY_NAME} STATIC
        src/Module.cpp)

target_include_directories(${CEC_DEVICE_ADAPTER_LIBRARY_NAME}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(${CEC_DEVICE_ADAPTER_LIBRARY_NAME}
    PUBLIC
        _cec_common_includes
        Threads::Threads
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging)

if("${CEC_DEVICE_ADAPTER_IMPLEMENTATION}" STREQUAL "Nexus")
    find_package(NEXUS REQUIRED)
    find_package(NXCLIENT REQUIRED)

    target_sources(${CEC_DEVICE_ADAPTER_LIBRARY_NAME}
        PRIVATE
            src/NexusAdapter.cpp)

    target_link_libraries(${CEC_DEVICE_ADAPTER_LIBRARY_NAME}
        PUBLIC
            NEXUS::NEXUS
            NXCLIENT::NXCLIENT)
endif()

if("${CEC_DEVICE_ADAPTER_IMPLEMENTATION}" STREQUAL "Linux")
    target_sources(${CEC_DEVICE_ADAPTER_LIBRARY_NAME}
        PRIVATE
            src/LinuxAdapter.cpp)
endif()

if("${CEC_DEVICE_ADAPTER_IMPLEMENTATION}" STREQUAL "Amlogic")
    target_sources(${CEC_DEVICE_ADAPTER_LIBRARY_NAME}
        PRIVATE
            src/AmlogicAdapter.cpp
            src/LogicAddressClaimer.cpp)

    target_link_libraries(${CEC_DEVICE_ADAPTER_LIBRARY_NAME}
        PUBLIC
            SimpleWorker::SimpleWorker)
endif()

if(CEC_TESTS)
    add_subdirectory(test)
endif()
