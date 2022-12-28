# - Try to find NANOMSG
# Once done this will define
#  NANOMSG_FOUND - System has nanomsg
#  NANOMSG_INCLUDE_DIRS - The nanomsg include directories
#  NANOMSG_LIBRARIES - The libraries needed to use nanomsg
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

if(NanoMsg_FIND_QUIETLY)
    set(_NANOMSG_MODE QUIET)
elseif(NanoMsg_FIND_REQUIRED)
    set(_NANOMSG_MODE REQUIRED)
endif()

find_package(PkgConfig)
pkg_check_modules(NANOMSG ${_NANOMSG_MODE} nanomsg)

find_library(NANOMSG_LIBRARY NAMES ${NANOMSG_LIBRARIES}
        HINTS ${NANOMSG_LIBDIR} ${NANOMSG_LIBRARY_DIRS}
        )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NanoMsg DEFAULT_MSG NANOMSG_LIBRARIES NANOMSG_LIBRARY)
mark_as_advanced(NANOMSG_INCLUDE_DIRS NANOMSG_LIBRARIES NANOMSG_LIBRARY)

if(NanoMsg_FOUND AND NOT TARGET NanoMsg::NanoMsg)
    add_library(NanoMsg::NanoMsg UNKNOWN IMPORTED)
    set_target_properties(NanoMsg::NanoMsg PROPERTIES
            IMPORTED_LOCATION "${NANOMSG_LIBRARY}"
            INTERFACE_LINK_LIBRARIES "${NANOMSG_LIBRARIES}"
            INTERFACE_COMPILE_OPTIONS "${NANOMSG_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${NANOMSG_INCLUDE_DIRS}"
            )
endif()
