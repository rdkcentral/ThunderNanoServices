# - Try to find Parodus2CCSP
# Once done this will define
#  PARODUS2CCSP_FOUND - System has Parodus2CCSP
#  PARODUS2CCSP_INCLUDE_DIRS - The Parodus2CCSP include directories
#  PARODUS2CCSP_LIBRARIES - The libraries needed to use Parodus2CCSP
#
# Copyright (C) 2019 Metrological.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE

# Parodus2CCSP has no pc file to search for

find_library(PARODUS2CCSP_LIBRARY webpa)

if(EXISTS "${PARODUS2CCSP_LIBRARY}")
    include(FindPackageHandleStandardArgs)

    set(PARODUS2CCSP_FOUND TRUE)

    find_package_handle_standard_args(PARODUS2CCSP DEFAULT_MSG PARODUS2CCSP_FOUND PARODUS2CCSP_LIBRARY)
    mark_as_advanced(PARODUS2CCSP_LIBRARY)

    if(NOT TARGET Parodus2CCSP::Parodus2CCSP)
        add_library(Parodus2CCSP::Parodus2CCSP UNKNOWN IMPORTED)
        set_target_properties(Parodus2CCSP::Parodus2CCSP PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${PARODUS2CCSP_LIBRARY}"
                )
    endif()
else()
    if(Parodus2CCSP_FIND_REQUIRED)
        message(FATAL_ERROR "PARODUS2CCSP_LIBRARY not available")
    elseif(NOT Parodus2CCSP_FIND_QUIETLY)
        message(STATUS "PARODUS2CCSP_LIBRARY not available")
    endif()
endif()
