# - Try to find WDMP-C
# Once done this will define
#  PROCPS_FOUND - System has WDMP-C
#  PROCPS_INCLUDE_DIRS - The WDMP-C include directories
#  PROCPS_LIBRARIES - The libraries needed to use WDMP-C
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

# PROCPS has no pc file to search for

find_library(PROCPS_LIBRARY procps)

if(EXISTS "${PROCPS_LIBRARY}")
    include(FindPackageHandleStandardArgs)

    set(PROCPS_FOUND TRUE)

    find_package_handle_standard_args(PROCPS DEFAULT_MSG PROCPS_FOUND PROCPS_LIBRARY)
    mark_as_advanced(PROCPS_LIBRARY)

    if(NOT TARGET procps::procps)
        add_library(procps::procps UNKNOWN IMPORTED)

        set_target_properties(procps::procps PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${PROCPS_LIBRARY}"
                )
    endif()
endif()
