# - Try to find BroadCom RefSW.
# Once done this will define
#  BCMREFSW_FOUND - System has Nexus
#  BCMREFSW_INCLUDE_DIRS - The Nexus include directories
#
#  All variable from platform_app.inc are available except:
#  - NEXUS_PLATFORM_VERSION_NUMBER.
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

find_library(LIBNXSERVER_LIBRARY NAMES libnxserver.a)

find_path(LIBNXSERVER_INCLUDE_DIR nxserverlib.h
        PATHS usr/include/
        PATH_SUFFIXES nxclient refsw)

find_file(NXSERVER_EXECUTABLE NAMES nxserver
        PATHS usr/bin/)

if( NOT NXSERVER_EXECUTABLE STREQUAL NXSERVER_EXECUTABLE-NOTFOUND  OR NOT LIBNXSERVER_LIBRARY STREQUAL LIBNXSERVER_LIBRARY-NOTFOUND )
        message(STATUS "Detected a nxserver...")
        set(NXSERVER_FOUND TRUE)
endif()

if(NOT LIBNXSERVER_LIBRARY STREQUAL LIBNXSERVER_LIBRARY-NOTFOUND)
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(LIBNEXUS DEFAULT_MSG LIBNXSERVER_INCLUDE_DIR LIBNXSERVER_LIBRARY)

    mark_as_advanced(LIBNXSERVER_INCLUDE_DIR LIBNXSERVER_LIBRARY)

    if(NOT TARGET NEXUS::NXSERVER)
        add_library(NEXUS::NXSERVER UNKNOWN IMPORTED)
        if(EXISTS "${LIBNEXUS_LIBRARY}")
            set_target_properties(NEXUS::NXSERVER PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                    IMPORTED_LOCATION "${LIBNXSERVER_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${LIBNXSERVER_INCLUDE_DIR}"
                        )
        endif()
    endif()
endif()
