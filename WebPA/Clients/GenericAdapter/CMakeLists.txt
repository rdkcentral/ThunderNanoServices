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

set(TARGET GenericAdapter)
message("Setup ${TARGET} v${VERSION}...")

find_package(WPEFramework)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(WDMP-C REQUIRED)
find_package(TinyXML REQUIRED)
find_package(LibParodus REQUIRED)
find_package(GLIB REQUIRED)


add_library(${TARGET}
    Handler/Handler.cpp
    Adapter/DataModel/DataModel.cpp
    Adapter/Notifier.cpp
    Adapter/Parameter.cpp
    Adapter/Attribute.cpp
    Adapter/Adapter.cpp
    Implementation.cpp
    Module.cpp
)

target_include_directories(${TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        Adapter
        Adapter/DataModel
        Handler
        ${GLIB_INCLUDE_DIRS}
)

target_link_libraries(${TARGET}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        wdmp_c::wdmp_c
        tinyxml::tinyxml
        libparodus::libparodus
        ${GLIB_LIBRARIES}
)

target_compile_definitions(${TARGET} PRIVATE ${PLUGIN_DEFINITIONS})
target_include_directories(${TARGET} PRIVATE ${PLUGIN_INCLUDE_DIRS})

set_target_properties(${TARGET} PROPERTIES SUFFIX ".webpa")
set_target_properties(${TARGET} PROPERTIES PREFIX "")
install(
    TARGETS ${TARGET}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/WebPA
)
install(
    FILES data-model.xml
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/WebPA)

add_subdirectory(Profiles)
