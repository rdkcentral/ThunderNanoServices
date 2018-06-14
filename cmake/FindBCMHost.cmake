# - Try to find bcm_host.
# Once done, this will define
#
#  BCM_HOST_INCLUDE_DIRS - the bcm_host include directories
#  BCM_HOST_LIBRARIES - link these to use bcm_host.
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

find_package(PkgConfig)
pkg_check_modules(PC_BCM_HOST bcm_host)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PC_BCM_HOST DEFAULT_MSG PC_BCM_HOST_LIBRARIES)

if(PC_BCM_HOST_FOUND)
    set(BCM_HOST_CFLAGS ${PC_BCM_HOST_CFLAGS})
    set(BCM_HOST_FOUND ${PC_BCM_HOST_FOUND})
    set(BCM_HOST_DEFINITIONS ${PC_BCM_HOST_CFLAGS_OTHER})
    set(BCM_HOST_NAMES ${PC_BCM_HOST_LIBRARIES})
    set(BCM_HOST_INCLUDE_DIRS ${PC_BCM_HOST_INCLUDE_DIRS})

    foreach (_library ${BCM_HOST_NAMES})
        find_library(BCM_HOST_LIBRARIES_${_library} ${_library}
                HINTS ${PC_BCM_HOST_LIBDIR} ${PC_BCM_HOST_LIBRARY_DIRS}
                )
        set(BCM_HOST_LIBRARIES ${BCM_HOST_LIBRARIES} ${BCM_HOST_LIBRARIES_${_library}})
    endforeach ()

    find_library(BCM_HOST_LIBRARY NAMES bcm_host
            HINTS ${PC_BCM_HOST_LIBDIR} ${PC_BCM_HOST_LIBRARY_DIRS}
            )

    find_library(OPENMAXIL_LIB NAMES openmaxil
            HINTS ${PC_BCM_HOST_LIBDIR} ${PC_BCM_HOST_LIBRARY_DIRS}
            )

    if(OPENMAXIL_LIB_FOUND)
        set(OPENMAXIL_LIBRARY OPENMAXIL_LIB)
    endif()

    set(BCM_HOST_ADDITIONAL_DEFINITIONS
            -DOMX_SKIP64BIT
            -DRPI_NO_X
            )

    if(BCM_HOST_FOUND AND NOT TARGET BROADCOM::HOST)
        add_library(BROADCOM::HOST UNKNOWN IMPORTED)
        set_target_properties(BROADCOM::HOST PROPERTIES
                IMPORTED_LOCATION "${BCM_HOST_LIBRARY}"
                INTERFACE_LINK_LIBRARIES "${BCM_HOST_LIBRARIES};${OPENMAXIL_LIBRARY}"
                INTERFACE_COMPILE_OPTIONS "${BCM_HOST_DEFINITIONS};${BCM_HOST_ADDITIONAL_DEFINITIONS}"
                INTERFACE_INCLUDE_DIRECTORIES "${BCM_HOST_INCLUDE_DIRS}"
                )
    endif()
    mark_as_advanced(BCM_HOST_INCLUDE_DIRS BCM_HOST_LIBRARIES)
endif()