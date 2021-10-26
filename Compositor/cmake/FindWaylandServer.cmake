# - Try to find Wayland.
# Once done this will define
#  WAYLANDSERVER_FOUND - System has Nexus
#  WAYLANDSERVER_INCLUDE_DIRS - The Nexus include directories
#  WAYLANDSERVER_LIBRARIES    - The libraries needed to use Nexus
#
#  WaylandServer::WaylandServer, the wayland server library
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

if(WaylandServer_FIND_QUIETLY)
    set(_WAYLAND_SERVER_MODE QUIET)
elseif(WaylandServer_FIND_REQUIRED)
    set(_WAYLAND_SERVER_MODE REQUIRED)
endif()

find_package(PkgConfig)
pkg_check_modules(WAYLANDSERVER ${_WAYLAND_SERVER_MODE} wayland-server)

find_library(WAYLANDSERVER_LIB NAMES wayland-client
        HINTS ${WAYLANDSERVER_LIBDIR} ${WAYLANDSERVER_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WaylandServer DEFAULT_MSG WAYLANDSERVER_FOUND WAYLANDSERVER_INCLUDE_DIRS WAYLANDSERVER_LIBRARIES WAYLANDSERVER_LIB)
mark_as_advanced(WAYLANDSERVER_INCLUDE_DIRS WAYLANDSERVER_LIBRARIES)

if(WaylandServer_FOUND AND NOT TARGET WaylandServer::WaylandServer)
    add_library(WaylandServer::WaylandServer UNKNOWN IMPORTED)
    set_target_properties(WaylandServer::WaylandServer PROPERTIES
            IMPORTED_LOCATION "${WAYLANDSERVER_LIB}"
            INTERFACE_LINK_LIBRARIES "${WAYLANDSERVER_LIBRARIES}"
            INTERFACE_COMPILE_OPTIONS "${WAYLANDSERVER_CFLAGS_OTHER}"
            INTERFACE_INCLUDE_DIRECTORIES "${WAYLANDSERVER_INCLUDE_DIRS}"
            )
endif()
