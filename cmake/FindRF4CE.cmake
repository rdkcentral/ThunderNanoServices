# - Try to find rf4ce support
# Once done this will define
#  RF4CE_FOUND - System has rf4ce support
#  RF4CE_INCLUDE_DIRS - The rf4ce include directories
#  RF4CE_LIBRARIES - The libraries needed to use rf4ce
#
# Copyright (C) 2017 Metrological B.V.
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
pkg_check_modules(RF4CE rf4ce)

if (PC_RF4CE_FOUND)
    message("Found rf4ce.pc")
    # FIXME this probably needs to set RF4CE_INCLUDE_DIRS and RF4CE_LIBRARIES from PC
else()
    find_path(LIBGP_INCLUDE_DIR gpVersion.h
        PATHS usr/include/
        PATH_SUFFIXES gprf4ce
        )

    find_library(LIBGP_LIBRARY NAMES RefTarget_ZRC_MSO_GP712_FEM_RDK_WorldBox)

    set(RF4CE_INCLUDE_DIRS ${LIBGP_INCLUDE_DIR})
    set(RF4CE_LIBRARIES ${LIBGP_LIBRARY})
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(RF4CE DEFAULT_MSG RF4CE_LIBRARIES RF4CE_INCLUDE_DIRS)

mark_as_advanced(RF4CE_FOUND RF4CE_INCLUDE_DIRS RF4CE_LIBRARIES)
