# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
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

set(TARGET Device)

find_package(WPEFramework REQUIRED)
find_package(Procps REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

file(GLOB IFACE_CONTROL_PLUGIN_INCLUDES *.h)

add_library(${TARGET}
    Module.cpp
    DeviceControl.cpp
    DeviceInfo.cpp
)

target_include_directories(${TARGET}
    PRIVATE
        ../
        ../../
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        procps::procps
)

set_target_properties(${TARGET} PROPERTIES SUFFIX ".profile")
set_target_properties(${TARGET} PROPERTIES PREFIX "")

install(TARGETS ${TARGET} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/WPEFramework/${PLUGIN_NAME})
