# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the License);
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# - Try to find LibDropbear
# Once done this will define
#  LIBDROPBEAR_FOUND - System has LibDropbear
#  LIBDROPBEAR_INCLUDE_DIRS - The LibDropbear include directories
#  LIBDROPBEAR_LIBRARIES - The libraries needed to use LibDropbear
#

message("<FindLibDropbear.cmake>")

find_package(PkgConfig)
pkg_check_modules(PC_LIBDROPBEAR REQUIRED LibDropbear)


if(PC_LIBDROPBEAR_FOUND)
    set(LIBDROPBEAR_INCLUDE_DIRS ${PC_LIBDROPBEAR_INCLUDEDIR})
    find_library(LIBDROPBEAR_LIBRARIES
        NAMES libdropbear.so
        HINTS ${PC_LIBDROPBEAR_LIBRARY_DIRS} ${PC_LIBDROPBEAR_LIBDIR}
    )
    set(LIBDROPBEAR_LIBRARY_DIRS ${PC_LIBDROPBEAR_LIBRARY_DIRS})
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBDROPBEAR DEFAULT_MSG LIBDROPBEAR_INCLUDE_DIRS LIBDROPBEAR_LIBRARIES)

mark_as_advanced(
    LIBDROPBEAR_FOUND
    LIBDROPBEAR_INCLUDE_DIRS
    LIBDROPBEAR_LIBRARIES
    LIBDROPBEAR_LIBRARY_DIRS)

message("</FindLibDropbear.cmake>")
