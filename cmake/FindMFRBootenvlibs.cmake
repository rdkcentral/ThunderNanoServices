# - Try to find MFRBOOTENVLIBS
# Once done this will define
#  MFRBOOTENVLIBS_FOUND - System has bootenvlibs
#  MFRBOOTENVLIBS_INCLUDE_DIRS - The bootenvlibs include directories
#  MFRBOOTENVLIBS_LIBRARIES - The libraries needed to use bootenvlibs
#
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

find_package(PkgConfig)
pkg_check_modules(MFRBOOTENVLIBS libbootenv)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(bootenvlibs DEFAULT_MSG MFRBOOTENVLIBS_LIBRARIES)

mark_as_advanced(MFRBOOTENVLIBS_INCLUDE_DIRS MFRBOOTENVLIBS_LIBRARIES)


find_library(MFRBOOTENVLIBS_LIBRARY NAMES ${MFRBOOTENVLIBS_LIBRARIES}
    HINTS ${MFRBOOTENVLIBS_LIBDIR} ${MFRBOOTENVLIBS_LIBRARY_DIRS}
    )

if(MFRBOOTENVLIBS_LIBRARY AND NOT TARGET bootenvlibs::bootenvlibs)
    add_library(bootenvlibs::bootenvlibs UNKNOWN IMPORTED)
    set_target_properties(bootenvlibs::bootenvlibs PROPERTIES
        IMPORTED_LOCATION "${MFRBOOTENVLIBS_LIBRARY}"
        INTERFACE_LINK_LIBRARIES "${MFRBOOTENVLIBS_LIBRARIES}"
        INTERFACE_COMPILE_OPTIONS "${MFRBOOTENVLIBS_DEFINITIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${MFRBOOTENVLIBS_INCLUDE_DIRS}"
        )
endif()
