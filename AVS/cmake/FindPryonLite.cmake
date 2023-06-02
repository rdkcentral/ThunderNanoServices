# If not stated otherwise in this file or this component's license file the
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

# - Try to find  PryonLite
# Once done this will define
#  PryonLite_FOUND - System has PryonLite
#  PRYON_LITE_INCLUDES - The PryonLite include directories
#  PRYON_LITE_LIBRARIES - The libraries needed to use PryonLite

find_path(PRYON_LITE_INCLUDES pryon_lite.h PATH_SUFFIXES pryon_lite)
find_library(PRYON_LITE_LIBRARIES libpryon_lite.so)

find_package_handle_standard_args(PryonLite DEFAULT_MSG
        PRYON_LITE_INCLUDES
        PRYON_LITE_LIBRARIES)
mark_as_advanced(PryonLite_FOUND PRYON_LITE_INCLUDES PRYON_LITE_LIBRARIES)

if(PryonLite_FOUND)
    if(NOT TARGET PryonLite::PryonLite)
        add_library(PryonLite::PryonLite SHARED IMPORTED)
        set_target_properties(PryonLite::PryonLite PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${PRYON_LITE_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${PRYON_LITE_INCLUDES}"
            INTERFACE_LINK_LIBRARIES "${PRYON_LITE_LIBRARIES}"
        )
    endif()
else()
    if(PryonLite_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find PryonLite")
    elseif(NOT PryonLite_FIND_QUIETLY)
        message(STATUS "Could NOT find PryonLite")
    endif()
endif()
