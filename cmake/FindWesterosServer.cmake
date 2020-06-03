# - Try to find BroadCom nxserver library.
# Once done this will define
#  WESTEROS_FOUND - System has westeros
#  WESTEROS_INCLUDE_DIRS - The westeros include directories
#  WESTEROS_LIBRARIES    - The libraries needed to use westeros
#
#  WESTEROS::WESTEROS, the westeros compositor
#
# Copyright (C) 2015 Metrological.
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
pkg_check_modules(PC_WESTEROS westeros-compositor)

find_library(WESTEROS_SERVER_LIB NAMES westeros_compositor
        HINTS ${PC_WESTEROS_LIBDIR} ${PC_WESTEROS_LIBRARY_DIRS}
        )

find_library(WESTEROS_SERVER_LIB_GL NAMES westeros_gl
        HINTS ${PC_WESTEROS_LIBDIR} ${PC_WESTEROS_LIBRARY_DIRS}
        )

find_library(WESTEROS_SERVER_LIB_BUFFER_SERVER NAMES westeros_simplebuffer_server
        HINTS ${PC_WESTEROS_LIBDIR} ${PC_WESTEROS_LIBRARY_DIRS}
        )

find_library(WESTEROS_SERVER_LIB_SHELL_SERVER NAMES westeros_simpleshell_server
        HINTS ${PC_WESTEROS_LIBDIR} ${PC_WESTEROS_LIBRARY_DIRS}
        )

find_path(WESTEROS_SERVER_INCLUDE_DIRS NAMES westeros-compositor.h
        HINTS ${PC_WESTEROS_INCLUDEDIR} ${PC_WESTEROS_INCLUDE_DIRS}
        )

set (WESTEROS_SERVER_LIBRARIES ${PC_WESTEROS_LIBRARIES} "${WESTEROS_SERVER_LIB_SERVER}" "${WESTEROS_SERVER_LIB_SERVER}")

set (WESTEROS_SERVER_FOUND ${PC_WESTEROS_FOUND})

message(WESTEROS_SERVER_LIB_GL)
message(${WESTEROS_SERVER_LIB_GL})

if(WESTEROS_SERVER_FOUND AND NOT TARGET WesterosServer::WesterosServer)
    set(WESTEROS_SERVER_LIB_SERVER_LINK_LIBRARIES "${WESTEROS_SERVER_LIB}" "${WESTEROS_SERVER_LIB_GL}" "${WESTEROS_SERVER_LIB_SHELL_SERVER}" "${WESTEROS_SERVER_LIB_BUFFER_SERVER}")
    add_library(WesterosServer::WesterosServer UNKNOWN IMPORTED)
    set_target_properties(WesterosServer::WesterosServer PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${WESTEROS_SERVER_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${WESTEROS_SERVER_INCLUDE_DIRS}"
            INTERFACE_COMPILE_OPTIONS "${WESTEROS_SERVER_CFLAGS_OTHER}"
            INTERFACE_LINK_LIBRARIES "${WESTEROS_SERVER_LIB_SERVER_LINK_LIBRARIES}"
            )
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PC_WESTEROS DEFAULT_MSG WESTEROS_SERVER_FOUND)

mark_as_advanced(WESTEROS_SERVER_INCLUDE_DIRS WESTEROS_LIBRARIES)
