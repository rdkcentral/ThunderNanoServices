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

# - Try to find  AlexaClientSDK
# Once done this will define
#  ALEXA_CLIENT_SDK_FOUND - System has AlexaClientSDK
#  ALEXA_CLIENT_SDK_INCLUDES - The AlexaClientSDK include directories
#  ALEXA_CLIENT_SDK_LIBRARIES - The libraries needed to use AlexaClientSDK

if(AlexaClientSDK_FIND_QUIETLY)
    set(_ALEXA_CLIENT_SDK_MODE QUIET)
elseif(AlexaClientSDK_FIND_REQUIRED)
    set(_ALEXA_CLIENT_SDK_MODE REQUIRED)
endif()

find_package(PkgConfig)
pkg_check_modules(PC_ALEXA_CLIENT_SDK ${_ALEXA_CLIENT_SDK_MODE} AlexaClientSDK)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AlexaClientSDK DEFAULT_MSG PC_ALEXA_CLIENT_SDK_FOUND)

mark_as_advanced(PC_ALEXA_CLIENT_SDK_INCLUDE_DIRS PC_ALEXA_CLIENT_SDK_LIBRARIES PC_ALEXA_CLIENT_SDK_CFLAGS_OTHER)

if(AlexaClientSDK_FOUND)
    set(ALEXA_CLIENT_SDK_FOUND ${PC_ALEXA_CLIENT_SDK_FOUND})
    set(ALEXA_CLIENT_SDK_INCLUDES ${PC_ALEXA_CLIENT_SDK_INCLUDE_DIRS})
    set(ALEXA_CLIENT_SDK_LIBRARIES ${PC_ALEXA_CLIENT_SDK_LIBRARIES})
    find_library(ALEXA_CLIENT_SDK_LIBRARY ${PC_ALEXA_CLIENT_SDK_LIBRARIES} ${PC_ALEXA_CLIENT_SDK_LIBRARY_DIRS})

    if(NOT TARGET AlexaClientSDK::AlexaClientSDK)
        add_library(AlexaClientSDK::AlexaClientSDK UNKNOWN IMPORTED)
        set_target_properties(AlexaClientSDK::AlexaClientSDK PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${ALEXA_CLIENT_SDK_LIBRARY}"
            INTERFACE_COMPILE_DEFINITIONS "${PC_ALEXA_CLIENT_SDK_DEFINITIONS}"
            INTERFACE_COMPILE_OPTIONS "${PC_ALEXA_CLIENT_SDK_CFLAGS_OTHER}"
            INTERFACE_INCLUDE_DIRECTORIES "${ALEXA_CLIENT_SDK_INCLUDES}"
            INTERFACE_LINK_LIBRARIES "${ALEXA_CLIENT_SDK_LIBRARIES}")
    endif()
endif()
