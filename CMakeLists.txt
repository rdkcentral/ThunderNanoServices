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

cmake_minimum_required(VERSION 3.15)
project(Extensions)

find_package(WPEFramework)

option(EXT_PLUGIN_INITIALIZER "Include Plugin Initializer Extension" OFF)
option(EXT_SYSTEMD_CONNECTOR "Include Systemd Connector Extension" OFF)

set(INTERFACES_PATTERNS "I*.h" CACHE STRING "Patterns matching files for which stubs should be generated")
set(JSONRPC_PATTERNS "*.json" CACHE STRING "Patterns matching files for which json stubs should be generated")

if(ENABLE_STRICT_COMPILER_SETTINGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "Compiling with Clang")
        set(CMAKE_STRICT_COMPILER_SETTINGS "-Weverything -Wextra -Wpedantic -Werror")
        set(CMAKE_STRICT_CXX_COMPILER_SETTINGS "${CMAKE_STRICT_COMPILER_SETTINGS} -Wnon-virtual-dtor")
    elseif(${CMAKE_COMPILER_IS_GNUCXX})
        message(STATUS "Compiling with GCC")
        set(CMAKE_STRICT_COMPILER_SETTINGS "-Wall -Wextra -Wpedantic -Werror")
        set(CMAKE_STRICT_CXX_COMPILER_SETTINGS "${CMAKE_STRICT_COMPILER_SETTINGS} -Wnon-virtual-dtor")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        message(STATUS "Compiling with MS Visual Studio")
        set(CMAKE_STRICT_COMPILER_SETTINGS "/W4")
    else()
        message(STATUS "Compiler ${CMAKE_CXX_COMPILER_ID}")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_STRICT_CXX_COMPILER_SETTINGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_STRICT_COMPILER_SETTINGS}")
endif()

add_subdirectory(interfaces)

if(EXT_PLUGIN_INITIALIZER)
    add_subdirectory(PluginInitializerService)
endif()

if(EXT_SYSTEMD_CONNECTOR)
	add_subdirectory(SystemdConnector)
endif()

