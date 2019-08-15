# - Try to find cjson
# Once done this will define
#  CJSON_FOUND - System has cjson
#  CJSON_INCLUDE_DIRS - The cjson include directories
#  CJSON_LIBRARIES - The libraries needed to use cjson
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

# cjson has no pc file to search for

find_path(CJSON_INCLUDE cJSON.h
    HINTS /usr/include/cjson /usr/local/include/cjson ${CMAKE_INSTALL_PREFIX}/usr/include/cjson)

find_library(CJSON_LIBRARY cjson)

if(EXISTS "${CJSON_LIBRARY}")
    include(FindPackageHandleStandardArgs)

    set(CJSON_FOUND TRUE)

    find_package_handle_standard_args(CJSON DEFAULT_MSG CJSON_FOUND CJSON_INCLUDE CJSON_LIBRARY)
    mark_as_advanced(CJSON_INCLUDE CJSON_LIBRARY)

    if(NOT TARGET cjson::cjson)
        add_library(cjson::cjson UNKNOWN IMPORTED)

        set_target_properties(cjson::cjson PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${CJSON_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${CJSON_INCLUDE}"
                )
    endif()
endif()
