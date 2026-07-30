# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2024 Metrological
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

cmake_minimum_required(VERSION 3.3)

project(ES1Benchmark)

find_package(WPEFramework)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

set(PLUGIN_ES1BENCHMARK_AUTOSTART "true" CACHE STRING "Automatically start ES1Benchmark plugin")

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

# -------------------------------------------------------------------
# Plugin shared library (loads into WPEFramework, runs in-process)
# -------------------------------------------------------------------
add_library(${MODULE_NAME} SHARED
    Module.cpp
    ES1Benchmark.cpp
)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
)

set_target_properties(${MODULE_NAME}
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
)

string(TOLOWER ${NAMESPACE} NAMESPACE_LIB)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${NAMESPACE_LIB}/plugins
    COMPONENT ${NAMESPACE}_Runtime
)

write_config()

# -------------------------------------------------------------------
# Standalone measurement client (optional, enable with -DPLUGIN_ES1BENCHMARK_CLIENT=ON)
# -------------------------------------------------------------------
option(PLUGIN_ES1BENCHMARK_CLIENT "Build ES1Benchmark standalone measurement client" OFF)

if(PLUGIN_ES1BENCHMARK_CLIENT)
    add_subdirectory(client)
endif()
