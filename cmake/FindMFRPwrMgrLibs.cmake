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
#
# - Try to find MFRPWRMGRLIBS
# Once done this will define
#  MFRPWRMGRLIBS_FOUND - System has power manager libraries
#  MFRPWRMGRLIBS_INCLUDE_DIRS - power manager include directories
#  MFRPWRMGRLIBS_LIBRARIES - The libraries needed to use power manager
#

find_package(PkgConfig)
pkg_check_modules(MFRPWRMGRLIBS libpwrmgr)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(pwrmgrlibs DEFAULT_MSG MFRPWRMGRLIBS_LIBRARIES)

mark_as_advanced(MFRPWRMGRLIBS_INCLUDE_DIRS MFRPWRMGRLIBS_LIBRARIES)


find_library(MFRPWRMGRLIBS_LIBRARY NAMES ${MFRPWRMGRLIBS_LIBRARIES}
    HINTS ${MFRPWRMGRLIBS_LIBDIR} ${MFRPWRMGRLIBS_LIBRARY_DIRS}
    )

if(MFRPWRMGRLIBS_LIBRARY AND NOT TARGET pwrmgrlibs::pwrmgrlibs)
    add_library(pwrmgrlibs::pwrmgrlibs UNKNOWN IMPORTED)
    set_target_properties(pwrmgrlibs::pwrmgrlibs PROPERTIES
        IMPORTED_LOCATION "${MFRPWRMGRLIBS_LIBRARY}"
        INTERFACE_LINK_LIBRARIES "${MFRPWRMGRLIBS_LIBRARIES}"
        INTERFACE_COMPILE_OPTIONS "${MFRPWRMGRLIBS_DEFINITIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${MFRPWRMGRLIBS_INCLUDE_DIRS}"
        )
endif()
