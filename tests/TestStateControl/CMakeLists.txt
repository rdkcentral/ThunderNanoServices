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

project(TestStateControl)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
    TestStateControl.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
    CXX_STANDARD ${CXX_STD}
    CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins
    COMPONENT ${NAMESPACE}_Runtime)

set(PLUGIN_TESTSTATECONTROL_STARTMODE "Activated" CACHE STRING "Startup mode of the TestStateControl plugin")
set(PLUGIN_TESTSTATECONTROL_RESUMED false CACHE BOOL "Start TestStateControl in resumed state")

write_config()

# ---------------------------------------------------------------------------
# In-process GTest suite for the TestStateControl plugin.
# Enable with -DTESTSTATECONTROL_TESTS=ON.
# ---------------------------------------------------------------------------
option(TESTSTATECONTROL_TESTS "Build in-process GTest suite for TestStateControl" OFF)
if(TESTSTATECONTROL_TESTS)
    add_subdirectory(tests)
endif()
