# - Try to find wayland-egl.
# Once done, this will define
#
#  WAYLAND_EGL_INCLUDE_DIRS - the wayland-egl include directories
#  WAYLAND_EGL_LIBRARIES - link these to use wayland-egl.
#
#  WaylandEGL::WaylandEGL, the wayland-egl library
#
# Copyright (C) 2015 Igalia S.L.
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

if(WaylandEGL_FIND_QUIETLY)
    set(_WAYLAND_EGL_MODE QUIET)
elseif(WaylandEGL_FIND_REQUIRED)
    set(_WAYLAND_EGL_MODE REQUIRED)
endif()

find_package(PkgConfig)
pkg_check_modules(WAYLAND_EGL ${_WAYLAND_EGL_MODE} wayland-egl)

find_library(WAYLAND_EGL_LIB NAMES wayland-egl
        HINTS ${WAYLAND_EGL_LIBDIR} ${WAYLAND_EGL_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WaylandEGL DEFAULT_MSG WAYLAND_EGL_FOUND WAYLAND_EGL_INCLUDE_DIRS WAYLAND_EGL_LIBRARIES WAYLAND_EGL_LIB)
mark_as_advanced(WAYLAND_EGL_INCLUDE_DIRS WAYLAND_EGL_LIBRARIES)

if(WaylandEGL_FOUND AND NOT TARGET WaylandEGL::WaylandEGL)
    add_library(WaylandEGL::WaylandEGL UNKNOWN IMPORTED)
    set_target_properties(WaylandEGL::WaylandEGL PROPERTIES
            IMPORTED_LOCATION "${WAYLAND_EGL_LIB}"
            INTERFACE_LINK_LIBRARIES "${WAYLAND_EGL_LIBRARIES}"
            INTERFACE_COMPILE_OPTIONS "${WAYLAND_EGL_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${WAYLAND_EGL_INCLUDE_DIRS}"
            )
endif()


