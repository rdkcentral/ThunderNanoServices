# - Try to find BroadCom nxserver library.
# Once done this will define
#  WAYLANDSERVER_FOUND - System has Nexus
#  WAYLANDSERVER_INCLUDE_DIRS - The Nexus include directories
#  WAYLANDSERVER_LIBRARIES    - The libraries needed to use Nexus
#
#  WAYLAND::SERVER, the wayland server library
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
pkg_check_modules(WAYLANDSERVER wayland-server)

find_library(WAYLANDSERVER_LIB NAMES wayland-client
        HINTS ${WAYLANDSERVER_LIBDIR} ${WAYLANDSERVER_LIBRARY_DIRS})

if(WAYLANDSERVER_FOUND AND NOT TARGET WAYLAND::SERVER)
    add_library(WAYLAND::SERVER UNKNOWN IMPORTED)
    set_target_properties(WAYLAND::SERVER PROPERTIES
            IMPORTED_LOCATION "${WAYLANDSERVER_LIB}"
            INTERFACE_LINK_LIBRARIES "${WAYLANDSERVER_LIBRARIES}"
            INTERFACE_COMPILE_OPTIONS "${WAYLANDSERVER_CFLAGS_OTHER}"
            INTERFACE_INCLUDE_DIRECTORIES "${WAYLANDSERVER_INCLUDE_DIRS}"
            )
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(WAYLANDSERVER DEFAULT_MSG WAYLANDSERVER_FOUND)
mark_as_advanced(WAYLANDSERVER_INCLUDE_DIRS WAYLANDSERVER_LIBRARIES)
