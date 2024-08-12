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

cmake_minimum_required(VERSION 3.15)

# Be compatible even if a newer CMake version is available
cmake_policy(VERSION 3.7...3.12)

if(gbm_FIND_QUIETLY)
    set(_gbm_MODE QUIET)
elseif(gbm_FIND_REQUIRED)
    set(_gbm_MODE REQUIRED)
endif()

find_package(PkgConfig)
if(${PKG_CONFIG_FOUND})

    # Just check if the gbm.pc exist, and create the PkgConfig::gbm target
    # No version requirement (yet)
    pkg_check_modules(gbm ${_gbm_MODE} IMPORTED_TARGET gbm)
    find_library(gbm_ACTUAL_LIBRARY NAMES gbm
        HINTS ${gbm_LIBRARY_DIRS})

    find_path(gbm_INCLUDE_DIR NAMES gbm.h
        HINTS ${gbm_INCLUDEDIR} ${gbm_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Unable to locate PkgConfig")
endif()

include(FindPackageHandleStandardArgs)
# Sets the FOUND variable to TRUE if all required variables are present and set
find_package_handle_standard_args(
    gbm
    REQUIRED_VARS
        gbm_ACTUAL_LIBRARY
        gbm_LIBRARIES
        gbm_INCLUDE_DIR
    VERSION_VAR
        gbm_VERSION
)
mark_as_advanced(gbm_INCLUDE_DIR gbm_LIBRARIES gbm_ACTUAL_LIBRARY)

if(gbm_FOUND AND NOT TARGET gbm::gbm)
    add_library(gbm::gbm UNKNOWN IMPORTED)
    set_target_properties(gbm::gbm PROPERTIES
        IMPORTED_LOCATION "${gbm_ACTUAL_LIBRARY}"
        INTERFACE_LINK_LIBRARIES "${gbm_LIBRARIES}"
        INTERFACE_COMPILE_OPTIONS "${gbm_CFLAGS}"
        INTERFACE_INCLUDE_DIRECTORIES "${gbm_INCLUDE_DIR}"
    )
endif()
