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

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(${NAMESPACE}Messaging CONFIG REQUIRED)
find_package(${NAMESPACE}LocalTracer CONFIG REQUIRED)

add_executable(transformationtest main.cpp)

add_subdirectory(ref_matrix)

set_target_properties(transformationtest PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(transformationtest
    PRIVATE
        common::include
        common::transformation
        ref_matrix
        ${NAMESPACE}LocalTracer::${NAMESPACE}LocalTracer
)

install(TARGETS transformationtest DESTINATION ${CMAKE_INSTALL_BINDIR}/${NAMESPACE}Tests COMPONENT ${NAMESPACE}_Test)
