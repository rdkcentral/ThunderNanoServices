# - Try to find libparodus
# Once done this will define
#  LIBPARODUS_FOUND - System has libparodus
#  LIBPARODUS_INCLUDE_DIRS - The libparodus include directories
#  LIBPARODUS_LIBRARIES - The libraries needed to use libparodus
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

# libparodus has no pc file to search for

find_path(LIBPARODUS_INCLUDE libparodus.h
    HINTS /usr/include/libparodus /usr/local/include/libparodus ${CMAKE_INSTALL_PREFIX}/usr/include/libparodus)

find_library(LIBPARODUS_LIBRARY libparodus)

if(EXISTS "${LIBPARODUS_LIBRARY}")
    include(FindPackageHandleStandardArgs)

    set(LIBPARODUS_FOUND TRUE)

    find_package_handle_standard_args(LIBPARODUS DEFAULT_MSG LIBPARODUS_FOUND LIBPARODUS_INCLUDE LIBPARODUS_LIBRARY)
    mark_as_advanced(LIBPARODUS_INCLUDE LIBPARODUS_LIBRARY)

    if(NOT TARGET libparodus::libparodus)
        add_library(libparodus::libparodus UNKNOWN IMPORTED)

        set_target_properties(libparodus::libparodus PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${LIBPARODUS_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${LIBPARODUS_INCLUDE}"
                )
    endif()
endif()
