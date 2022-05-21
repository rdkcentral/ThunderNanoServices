# - Try to find WRP-C
# Once done this will define
#  WRP-C_FOUND - System has WRP-C
#  WRP_C_INCLUDE_DIRS - The WRP-C include directories
#  WRP_C_LIBRARIES - The libraries needed to use WRP-C
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

# WRP_C has no pc file to search for

find_path(WRP_C_INCLUDE wrp-c.h
    HINTS /usr/include/wrp-c /usr/local/include/wrp-c ${CMAKE_INSTALL_PREFIX}/usr/include/wrp-c)

find_library(WRP_C_LIBRARY wrp-c)

if(EXISTS "${WRP_C_LIBRARY}")
    include(FindPackageHandleStandardArgs)

    find_package_handle_standard_args(WRP-C DEFAULT_MSG WRP_C_INCLUDE WRP_C_LIBRARY)
    mark_as_advanced(WRP_C_INCLUDE WRP_C_LIBRARY)

    if(WRP-C_FOUND AND NOT TARGET WRP-C::WRP-C)
        add_library(WRP-C::WRP-C UNKNOWN IMPORTED)

        set_target_properties(WRP-C::WRP-C PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${WRP_C_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${WRP_C_INCLUDE}"
                )
    endif()
else()
    if(WRP-C_FIND_REQUIRED)
        message(FATAL_ERROR "WRP_C_LIBRARY not available")
    elseif(NOT WRP-C_FIND_QUIETLY)
        message(STATUS "WRP_C_LIBRARY not available")
    endif()

endif()
