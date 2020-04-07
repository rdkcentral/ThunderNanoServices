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

# - Try to find udev
# Once done this will define
#  UDEV_FOUND - System has udev
#  UDEV_INCLUDE_DIRS - The udev include directories
#  UDEV_LIBRARIES - The libraries needed to use udev

#udev has no pc file to search for

find_package(PkgConfig)
pkg_check_modules(PC_UDEV udev)

if (PC_UDEV_FOUND)
    set(UDEV_FOUND_TEXT "found")
else()
    set(UDEV_FOUND_TEXT "Not found")
endif()

find_library(UDEV_LIBRARY
    NAMES udev
)

if (WPEFRAMEWORK_VERBOSE_BUILD)
    message(STATUS "udev       : ${UDEV_FOUND_TEXT}")
    message(STATUS "  version      : ${PC_UDEV_VERSION}")
    message(STATUS "  cflags       : ${PC_UDEV_CFLAGS}")
    message(STATUS "  cflags other : ${PC_UDEV_CFLAGS_OTHER}")
    message(STATUS "  include dirs : ${PC_UDEV_INCLUDE_DIRS} ${PC_UDEV_INCLUDEDIR} ${UDEV_LIBRARY}")
    message(STATUS "  libs         : ${PC_UDEV_LIBRARIES} ${PC_UDEV_LIBRARY}")
endif()

set(UDEV_DEFINITIONS ${PC_UDEV_PLUGINS_CFLAGS_OTHER})
set(UDEV_INCLUDE_DIRS ${PC_UDEV_INCLUDEDIR})
set(UDEV_LIBRARIES "${UDEV_LIBRARY}" )
set(UDEV_LIBRARY_DIRS "${PC_UDEV_LIBRARY_DIRS} ${PC_UDEV_LIBDIR}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UDEV DEFAULT_MSG
    UDEV_LIBRARIES UDEV_INCLUDE_DIRS)

if(PC_UDEV_FOUND)
   set(UDEV_FOUND PC_UDEV_FOUND)
else()
    message(WARNING "Could not find udev")
endif()

mark_as_advanced(UDEV_DEFINITIONS UDEV_INCLUDE_DIRS UDEV_LIBRARIES)
