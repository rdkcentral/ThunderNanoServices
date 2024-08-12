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

find_package(Thunder)

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}LocalTracer REQUIRED)

add_executable(cec_accessor_test
    cec_accessor_test.cpp
)

set_target_properties(cec_accessor_test PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        ENABLE_EXPORTS FALSE)

target_link_libraries(cec_accessor_test PRIVATE
    ${CECADAPTER_LIBRARY_NAME}
    CompileSettingsDebug::CompileSettingsDebug
    ${NAMESPACE}Core::${NAMESPACE}Core
    ${NAMESPACE}LocalTracer::${NAMESPACE}LocalTracer)

install(TARGETS cec_accessor_test DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Test)
