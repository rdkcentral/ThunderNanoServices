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

# - Try to find libsqlite3-dev
# Once done this will define
#  LIBSQLITE3_FOUND - System has libsqlite3-dev
#  LIBSQLITE3_INCLUDE_DIRS - The libsqlite3-dev include directories
#  LIBSQLITE3_LIBRARIES - The libraries needed to use libsqlite3-dev

find_package(PkgConfig)
pkg_check_modules(PC_LIBSQLITE3 sqlite3)

if(PC_LIBSQLITE3_FOUND)
    if(LIBSQLITE3_FIND_VERSION AND PC_LIBSQLITE3_VERSION)
        if ("${LIBSQLITE3_FIND_VERSION}" VERSION_GREATER "${PC_LIBSQLITE3_VERSION}")
            message(WARNING "Incorrect version, found ${PC_LIBSQLITE3_VERSION}, need at least ${WPEFRAMEWORK_FIND_VERSION}, please install correct version ${LIBSQLITE3_FIND_VERSION}")
            set(LIBSQLITE3_FOUND_TEXT "Found incorrect version")
            unset(PC_LIBSQLITE3_FOUND)
        endif()
    endif()
endif()

if(PC_LIBSQLITE3_FOUND)
    set(LIBSQLITE3_FOUND_TEXT "Found")
else()
    set(LIBSQLITE3_FOUND_TEXT "Not found")
endif()

if (WPEFRAMEWORK_VERBOSE_BUILD)
    message(STATUS "libsqlite3     : ${LIBSQLITE3_FOUND_TEXT}")
    message(STATUS "  version      : ${PC_LIBSQLITE3_VERSION}")
    message(STATUS "  cflags       : ${PC_LIBSQLITE3_CFLAGS}")
    message(STATUS "  cflags other : ${PC_LIBSQLITE3_CFLAGS_OTHER}")
    message(STATUS "  include dirs : ${PC_LIBSQLITE3_INCLUDE_DIRS} ${PC_LIBSQLITE3_INCLUDEDIR}")
    message(STATUS "  lib dirs     : ${PC_LIBSQLITE3_LIBRARY_DIRS} ${PC_LIBSQLITE3_LIBDIR}")
    message(STATUS "  libs         : ${PC_LIBSQLITE3_LIBRARIES} ${PC_LIBSQLITE3_LIBRARY}")
endif()

set(LIBSQLITE3_DEFINITIONS ${PC_LIBSQLITE3_PLUGINS_CFLAGS_OTHER})
set(LIBSQLITE3_INCLUDE_DIR ${PC_LIBSQLITE3_INCLUDE_DIRS} ${PC_LIBSQLITE3_INCLUDEDIR})
set(LIBSQLITE3_INCLUDE_DIRS ${LIBSQLITE3_INCLUDE_DIR})
set(LIBSQLITE3_LIBRARIES ${PC_LIBSQLITE3_LIBRARIES} ${PC_LIBSQLITE3_LIBRARY})
set(LIBSQLITE3_LIBRARY ${LIBSQLITE3_LIBRARIES})
set(LIBSQLITE3_LIBRARY_DIRS ${PC_LIBSQLITE3_LIBRARY_DIRS} ${PC_LIBSQLITE3_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBSQLITE3 DEFAULT_MSG
    LIBSQLITE3_LIBRARIES LIBSQLITE3_INCLUDE_DIR)

if(LIBSQLITE3_FOUND)
else()
    message(WARNING "Could not find libsqlite3-dev")
endif()

mark_as_advanced(LIBSQLITE3_DEFINITIONS LIBSQLITE3_INCLUDE_DIRS LIBSQLITE3_LIBRARIES)
