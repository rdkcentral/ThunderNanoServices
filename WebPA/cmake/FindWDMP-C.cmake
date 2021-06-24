# - Try to find WDMP-C
# Once done this will define
#  WDMP_C_FOUND - System has WDMP-C
#  WDMP_C_INCLUDE_DIRS - The WDMP-C include directories
#  WDMP_C_LIBRARIES - The libraries needed to use WDMP-C
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

# WDMP_C has no pc file to search for

find_path(WDMP_C_INCLUDE wdmp-c.h
    HINTS /usr/include/wdmp-c /usr/local/include/wdmp-c ${CMAKE_INSTALL_PREFIX}/usr/include/wdmp-c)

find_library(WDMP_C_LIBRARY wdmp-c)

if(EXISTS "${WDMP_C_LIBRARY}")
    include(FindPackageHandleStandardArgs)

    set(WDMP_C_FOUND TRUE)

    find_package_handle_standard_args(WDMP_C DEFAULT_MSG WDMP_C_FOUND WDMP_C_INCLUDE WDMP_C_LIBRARY)
    mark_as_advanced(WDMP_C_INCLUDE WDMP_C_LIBRARY)

    if(NOT TARGET wdmp_c::wdmp_c)
        add_library(wdmp_c::wdmp_c UNKNOWN IMPORTED)

        set_target_properties(wdmp_c::wdmp_c PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${WDMP_C_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${WDMP_C_INCLUDE}"
                )
    endif()
endif()
