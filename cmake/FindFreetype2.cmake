# - Try to find Freetype2
# Once done this will define
#  FREETYPE2_FOUND - System has Freetype2
#  FREETYPE2_INCLUDE_DIRS - The Freetype2include directories
#  FREETYPE2_LIBRARIES - The libraries needed to use Freetype2
#
# Copyright (C) 2018 Metrological.
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
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

find_package(PkgConfig)
pkg_check_modules(PC_FREETYPE2 QUIET freetype2)

find_path(FREETYPE2_INCLUDE_DIR ft2build.h
           PATHS usr/include
           PATH_SUFFIXES freetype2
           HINTS ${PC_FREETYPE2_INCDIRS} ${PC_FREETYPE2_INCLUDE_DIRS})

set(FREETYPE2_INCLUDE_DIRS ${FREETYPE2_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(FREETYPE2 DEFAULT_MSG FREETYPE2_INCLUDE_DIRS)

mark_as_advanced(FREETYPE2_INCLUDE_DIRS)

find_library(FREETYPE2_LIBRARY NAMES ${PC_FREETYPE2_LIBRARIES}
        HINTS ${PC_FREETYPE2_LIBDIR} ${PC_FREETYPE2_LIBRARY_DIRS}
        )

if(FREETYPE2_LIBRARY AND NOT TARGET Freetype2::Freetype2)
    add_library(Freetype2::Freetype2 UNKNOWN IMPORTED)
    set_target_properties(Freetype2::Freetype2 PROPERTIES
            IMPORTED_LOCATION "${FREETYPE2_LIBRARY}"
            INTERFACE_LINK_LIBRARIES "${PC_FREETYPE2_LIBRARIES}"
            INTERFACE_COMPILE_OPTIONS "${PC_FREETYPE2_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE2_INCLUDE_DIRS}"
            )
endif()