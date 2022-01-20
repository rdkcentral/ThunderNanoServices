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

# - Try to find Asio
# Once done this will define
#  ASIO_FOUND - System has Asio
#  ASIO_INCLUDES - The Asio include directories

find_path(ASIO_INCLUDES asio.hpp)

find_package_handle_standard_args(Asio DEFAULT_MSG
        ASIO_INCLUDES)
mark_as_advanced(ASIO_INCLUDES)

if(Asio_FOUND)
    if(NOT TARGET Asio::Asio)
        add_library(Asio::Asio SHARED IMPORTED)
        set_target_properties(Asio::Asio
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${ASIO_INCLUDES}"
        )

    endif()
else()
    if(Asio_FIND_REQUIRED)
        message(FATAL_ERROR "Could NOT find Asio")
    elseif(NOT Asio_FIND_QUIETLY)
        message(STATUS "Could NOT find Asio")
    endif()
endif()
