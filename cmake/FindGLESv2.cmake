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

# - Try to Find GLESv2
# Once done, this will define
#
#  GLESV2_FOUND - system has EGL installed.
#  GLESV2_INCLUDE_DIRS - directories which contain the EGL headers.
#  GLESV2_LIBRARIES - libraries required to link against EGL.
#  GLESV2_DEFINITIONS - Compiler switches required for using EGL.
#

find_package(PkgConfig)

pkg_check_modules(PC_GLESV2 glesv2)

if (PC_GLESV2_FOUND)
    set(GLESV2_CFLAGS ${PC_GLESV2_CFLAGS})
    set(GLESV2_FOUND ${PC_GLESV2_FOUND})
    set(GLESV2_DEFINITIONS ${PC_GLESV2_CFLAGS_OTHER})
    set(GLESV2_NAMES ${PC_GLESV2_LIBRARIES})
    foreach (_library ${GLESV2_NAMES})
        find_library(GLESV2_LIBRARIES_${_library} ${_library}
	    HINTS ${PC_GLESV2_LIBDIR} ${PC_GLESV2_LIBRARY_DIRS}
        )
        set(GLESV2_LIBRARIES ${GLESV2_LIBRARIES} ${GLESV2_LIBRARIES_${_library}})
    endforeach ()
    set(GLESV2_INCLUDE_DIRS ${PC_GLESV2_INCLUDE_DIRS} CACHE FILEPATH "FIXME")
endif ()

find_library(GLESV2_LIBRARY NAMES GLESv2
        HINTS ${PC_EGL_LIBDIR} ${PC_EGL_LIBRARY_DIRS}
        )

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLESV2 DEFAULT_MSG GLESV2_FOUND)

if(GLESV2_FOUND AND NOT TARGET GLESV2::GLESV2)
    add_library(GLESV2::GLESV2 UNKNOWN IMPORTED)
    set_target_properties(GLESV2::GLESV2 PROPERTIES
            IMPORTED_LOCATION "${GLESV2_LIBRARY}"
            INTERFACE_LINK_LIBRARIES "${GLESV2_LIBRARIES}"
            INTERFACE_COMPILE_OPTIONS "${GLESV2_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${GLESV2_INCLUDE_DIRS}"
            )
endif()


mark_as_advanced(GLESV2_INCLUDE_DIRS GLESV2_LIBRARIES)
