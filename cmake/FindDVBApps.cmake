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

# - Try to find dvb-apps
# Once done this will define
#  DVBAPPS_FOUND - System has dvb-apps
#  DVBAPPS_INCLUDE_DIRS - The dvb-apps include directories
#  DVBAPPS_LIBRARIES - The libraries needed to use dvb-apps

#dvb-apps has no pc file to search for

find_path(
    LINUX_UCSI_INCLUDE_DIR
    NAMES libucsi/types.h
    HINTS /usr/include /usr/local/include ${CMAKE_INSTALL_PREFIX}/usr/include)

find_path(
    DVBAPPS_INCLUDE_DIR
    NAMES dvbfe.h
    PATH_SUFFIXES libdvbapi
)

find_library(
    DVBAPPS_LIBRARY
    NAMES dvbapi
    HINTS /usr/lib /usr/local/lib ${CMAKE_INSTALL_PREFIX}/usr/lib)

find_library(
    LINUX_UCSI_LIBRARY
    NAMES ucsi
    PATH_SUFFIXES libucsi
)

if("${DVBAPPS_INCLUDE_DIR}" STREQUAL "" OR "${DVBAPPS_LIBRARY}" STREQUAL "")
    set(DVBAPPS_FOUND_TEXT "Not found")
else()
    set(DVBAPPS_FOUND_TEXT "Found")
endif()

if (WPEFRAMEWORK_VERBOSE_BUILD)
    message(STATUS "dvb-apps       : ${DVBAPPS_FOUND_TEXT}")
    message(STATUS "  version      : ${PC_DVBAPPS_VERSION}")
    message(STATUS "  cflags       : ${PC_DVBAPPS_CFLAGS}")
    message(STATUS "  cflags other : ${PC_DVBAPPS_CFLAGS_OTHER}")
    message(STATUS "  include dirs : ${DVBAPPS_INCLUDE_DIR}")
    message(STATUS "  libs         : ${DVBAPPS_LIBRARY}")
endif()

set(DVBAPPS_DEFINITIONS ${PC_DVBAPPS_PLUGINS_CFLAGS_OTHER})
set(DVBAPPS_INCLUDE_DIRS ${DVBAPPS_INCLUDE_DIR} ${LINUX_UCSI_INCLUDE_DIR})
set(DVBAPPS_LIBRARIES ${DVBAPPS_LIBRARY} ${LINUX_UCSI_LIBRARY} ${UDEV_LIBRARY})
set(DVBAPPS_LIBRARY_DIRS ${PC_DVBAPPS_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DVBAPPS DEFAULT_MSG
    DVBAPPS_LIBRARIES DVBAPPS_INCLUDE_DIRS)

if(DVBAPPS_FOUND)
else()
    message(WARNING "Could not find dvb-apps")
endif()

mark_as_advanced(DVBAPPS_DEFINITIONS DVBAPPS_INCLUDE_DIRS DVBAPPS_LIBRARIES)
