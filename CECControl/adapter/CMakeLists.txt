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

find_package(Thunder)
find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(${NAMESPACE}Messaging REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_subdirectory(include)
add_subdirectory(processor)

option(CEC_TESTS "Include CEC test apps" OFF)

set(CEC_DEVICE_ADAPTER_LIBRARY_NAME "_ceccontrol_device_adapter" CACHE INTERNAL "INTERNALLY USED")
add_subdirectory(device_adapter)

add_library(${CECADAPTER_LIBRARY_NAME} STATIC
    Module.cpp
    CECEnumerate.cpp
    CECAdapter.cpp
    CECAccessor.cpp
)

target_link_libraries(${CECADAPTER_LIBRARY_NAME}
    PUBLIC
        ${CEC_DEVICE_ADAPTER_LIBRARY_NAME}
        _cec_common_includes
        _cec_message_processor
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging)

if(CEC_TESTS)
    add_subdirectory(test)
endif()
