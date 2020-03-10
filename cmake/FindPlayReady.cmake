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

# - Try to find PlayReady
# Once done this will define
#  PLAYREADY_FOUND - System has PlayReady
#  PLAYREADY_INCLUDE_DIRS - The PlayReady include directories
#  PLAYREADY_LIBRARIES - The libraries needed to use PlayReady
#  PLAYREADY_FLAGS - The flags needed to use PlayReady
#

find_package(PkgConfig)
pkg_check_modules(PC_PLAYREADY playready)

if(PC_PLAYREADY_FOUND)
    if(PLAYREADY_FIND_VERSION AND PC_PLAYREADY_VERSION)
        if ("${PLAYREADY_FIND_VERSION}" VERSION_GREATER "${PC_PLAYREADY_VERSION}")
            message(WARNING "Incorrect version, found ${PC_PLAYREADY_VERSION}, need at least ${PLAYREADY_FIND_VERSION}, please install correct version ${PLAYREADY_FIND_VERSION}")
            set(PLAYREADY_FOUND_TEXT "Found incorrect version")
            unset(PC_PLAYREADY_FOUND)
        endif()
    endif()

    if(PC_PLAYREADY_FOUND)

        # CPFLAGS += -DDRM_BUILD_PROFILE=DRM_BUILD_PROFILE_OEM  
        set(PLAYREADY_FLAGS ${PC_PLAYREADY_CFLAGS_OTHER} -DTARGET_SUPPORTS_UNALIGNED_DWORD_POINTERS=0 -DTARGET_LITTLE_ENDIAN=1)
        set(PLAYREADY_INCLUDE_DIRS ${PC_PLAYREADY_INCLUDE_DIRS})
        set(PLAYREADY_LIBRARIES ${PC_PLAYREADY_LIBRARIES})
        set(PLAYREADY_LIBRARY_DIRS ${PC_PLAYREADY_LIBRARY_DIRS})
    endif()
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PLAYREADY DEFAULT_MSG PLAYREADY_INCLUDE_DIRS PLAYREADY_LIBRARIES)

mark_as_advanced(
    PLAYREADY_FOUND
    PLAYREADY_INCLUDE_DIRS
    PLAYREADY_LIBRARIES
    PLAYREADY_LIBRARY_DIRS
    PLAYREADY_FLAGS)
