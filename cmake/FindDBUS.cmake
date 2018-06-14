# - Try to find DBUS
# Once done this will define
#  DBUS_FOUND - System has dbus
#  DBUS_INCLUDE_DIRS - The dbus include directories
#  DBUS_LIBRARIES - The libraries needed to use dbus
#
# Copyright (C) 2018 TATA ELXSI
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
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE

find_package(PkgConfig)

pkg_check_modules(PC_DBUS QUIET dbus-1)

if (PC_DBUS_FOUND)
    set(DBUS_CFLAGS ${PC_DBUS_CFLAGS})
    set(DBUS_FOUND ${PC_DBUS_FOUND})
    set(DBUS_DEFINITIONS ${PC_DBUS_CFLAGS_OTHER})
    set(DBUS_NAMES ${PC_DBUS_LIBRARIES})
    foreach (_library ${DBUS_NAMES})
        find_library(DBUS_LIBRARIES_${_library} ${_library}
        HINTS ${PC_DBUS_LIBDIR} ${PC_DBUS_LIBRARY_DIRS}
        )
        set(DBUS_LIBRARIES ${DBUS_LIBRARIES} ${DBUS_LIBRARIES_${_library}})
    endforeach ()
else ()
    set(DBUS_NAMES ${DBUS_NAMES} dbus DBUS)
    find_library(DBUS_LIBRARIES NAMES ${DBUS_NAMES}
        HINTS ${PC_DBUS_LIBDIR} ${PC_DBUS_LIBRARY_DIRS}
    )
endif ()

find_path(DBUS_INCLUDE_DIR_DBUS NAMES dbus/dbus.h
    HINTS ${PC_DBUS_INCLUDEDIR} ${PC_DBUS_INCLUDE_DIRS}
)
find_path(DBUS_INCLUDE_DIR_DBUS_ARCH NAMES dbus/dbus-arch-deps.h
    HINTS ${PC_DBUS_INCLUDEDIR} ${PC_DBUS_INCLUDE_DIRS}
)

set(DBUS_INCLUDE_DIRS ${DBUS_INCLUDE_DIR_DBUS} ${DBUS_INCLUDE_DIR_DBUS_ARCH} CACHE FILEPATH "FIXME")
set(DBUS_LIBRARIES ${DBUS_LIBRARIES} CACHE FILEPATH "FIXME")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(DBUS DEFAULT_MSG DBUS_INCLUDE_DIRS DBUS_LIBRARIES)

mark_as_advanced(DBUS_INCLUDE_DIRS DBUS_LIBRARIES)

