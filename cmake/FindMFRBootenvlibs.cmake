# - Try to find MFRBOOTENVLIBS
# Once done this will define
#  MFRBOOTENVLIBS_FOUND - System has bootenvlibs
#  MFRBOOTENVLIBS_INCLUDE_DIRS - The bootenvlibs include directories
#  MFRBOOTENVLIBS_LIBRARIES - The libraries needed to use bootenvlibs
#
# Copyright (C) 2019 RDK.
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
pkg_check_modules(MFRBOOTENVLIBS libbootenv)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(bootenvlibs DEFAULT_MSG MFRBOOTENVLIBS_LIBRARIES)

mark_as_advanced(MFRBOOTENVLIBS_INCLUDE_DIRS MFRBOOTENVLIBS_LIBRARIES)


find_library(MFRBOOTENVLIBS_LIBRARY NAMES ${MFRBOOTENVLIBS_LIBRARIES}
    HINTS ${MFRBOOTENVLIBS_LIBDIR} ${MFRBOOTENVLIBS_LIBRARY_DIRS}
    )

if(MFRBOOTENVLIBS_LIBRARY AND NOT TARGET bootenvlibs::bootenvlibs)
    add_library(bootenvlibs::bootenvlibs UNKNOWN IMPORTED)
    set_target_properties(bootenvlibs::bootenvlibs PROPERTIES
        IMPORTED_LOCATION "${MFRBOOTENVLIBS_LIBRARY}"
        INTERFACE_LINK_LIBRARIES "${MFRBOOTENVLIBS_LIBRARIES}"
        INTERFACE_COMPILE_OPTIONS "${MFRBOOTENVLIBS_DEFINITIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${MFRBOOTENVLIBS_INCLUDE_DIRS}"
        )
endif()
