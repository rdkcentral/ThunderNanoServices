# - Try to find TINYXML
# Once done this will define
#  TINYXML_FOUND - System has tinyxml
#  TINYXML_INCLUDE_DIRS - The tinyxml include directories
#  TINYXML_LIBRARIES - The libraries needed to use tinyxml
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

find_package(PkgConfig)
pkg_check_modules(TINYXML tinyxml)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(tinyxml DEFAULT_MSG TINYXML_LIBRARIES)

mark_as_advanced(TINYXML_INCLUDE_DIRS TINYXML_LIBRARIES)


find_library(TINYXML_LIBRARY NAMES ${TINYXML_LIBRARIES}
        HINTS ${TINYXML_LIBDIR} ${TINYXML_LIBRARY_DIRS}
        )

if(TINYXML_LIBRARY AND NOT TARGET tinyxml::tinyxml)
    add_library(tinyxml::tinyxml UNKNOWN IMPORTED)
    set_target_properties(tinyxml::tinyxml PROPERTIES
            IMPORTED_LOCATION "${TINYXML_LIBRARY}"
            INTERFACE_LINK_LIBRARIES "${TINYXML_LIBRARIES}"
            INTERFACE_COMPILE_OPTIONS "${TINYXML_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${TINYXML_INCLUDE_DIRS}"
            )
endif()
