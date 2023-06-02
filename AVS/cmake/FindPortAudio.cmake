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

# - Try to find  PortAudio
# Once done this will define
#  PORTAUDIO_FOUND - System has PortAudio
#  PORTAUDIO_INCLUDES - The PortAudio include directories
#  PORTAUDIO_LIBRARIES - The libraries needed to use PortAudio

if(PortAudio_FIND_QUIETLY)
    set(_PortAudio_MODE QUIET)
elseif(PortAudio_FIND_REQUIRED)
    set(_PortAudio_MODE REQUIRED)
endif()

find_package(PkgConfig)
pkg_check_modules(PC_PORTAUDIO ${_PortAudio_MODE} portaudio-2.0)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PortAudio DEFAULT_MSG PC_PORTAUDIO_FOUND)

mark_as_advanced(PC_PORTAUDIO_INCLUDE_DIRS PC_PORTAUDIO_LIBRARIES PC_PORTAUDIO_CFLAGS_OTHER)
if(PortAudio_FOUND)
    set(PORTAUDIO_FOUND ${PC_PORTAUDIO_FOUND})
    set(PORTAUDIO_INCLUDES ${PC_PORTAUDIO_INCLUDE_DIRS})
    set(PORTAUDIO_LIBRARIES ${PC_PORTAUDIO_LIBRARIES})

    if(NOT TARGET PortAudio::PortAudio)
        add_library(PortAudio::PortAudio SHARED IMPORTED)
        set_target_properties(PortAudio::PortAudio
            PROPERTIES
                INTERFACE_COMPILE_DEFINITIONS "${PC_PORTAUDIO_DEFINITIONS}"
                INTERFACE_COMPILE_OPTIONS "${PC_PORTAUDIO_CFLAGS_OTHER}"
                INTERFACE_INCLUDE_DIRECTORIES "${PC_PORTAUDIO_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES "${PC_PORTAUDIO_LIBRARIES}"
        )
    endif()
endif()
