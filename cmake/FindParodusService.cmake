# - Try to find parodus
# Once done this will define
#  PARODUS_SERVICE_FOUND - System has parodus
#  PARODUS_SERVICE_INCLUDE_DIRS - The parodus include directories
#  PARODUS_SERVICE_LIBRARIES - The libraries needed to use parodus
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

# parodus has no pc file to search for

find_library(PARODUS_SERVICE_LIBRARY parodus)

if(EXISTS "${PARODUS_SERVICE_LIBRARY}")
    include(FindPackageHandleStandardArgs)

    set(PARODUS_SERVICE_FOUND TRUE)

    find_package_handle_standard_args(ParodusService DEFAULT_MSG PARODUS_SERVICE_FOUND PARODUS_SERVICE_LIBRARY)
    mark_as_advanced(PARODUS_SERVICE_LIBRARY)

    if(NOT TARGET ParodusService::ParodusService)
        add_library(ParodusService::ParodusService UNKNOWN IMPORTED)

        set_target_properties(ParodusService::ParodusService PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${PARODUS_SERVICE_LIBRARY}"
                )
    endif()
endif()
