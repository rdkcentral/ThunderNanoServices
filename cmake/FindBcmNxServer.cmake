# - Try to find BroadCom nxserver library.
# Once done this will define
#  LIBNXSERVER_FOUND - System has Nexus
#  LIBNXSERVER_INCLUDE_DIRS - The Nexus include directories
#  LIBNXSERVER_LIBRARIES    - The libraries needed to use Nexus
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

find_library(LIBNEXUS_LIBRARY NAMES nexus)

find_library(LIBNXCLIENT_LIBRARY NAMES nxclient)

find_path(LIBNXSERVER_INCLUDE_DIR refsw/nxserverlib.h
        PATHS usr/include/
        )

find_path(NXREFSW_INCLUDE_PATH nexus_config.h
        PATHS usr/include/
        PATH_SUFFIXES refsw
        )

find_path(LIBNXCLIENT_INCLUDE_DIR refsw/nxclient.h
        PATHS usr/include/
        )

set(LIBNXSERVER_LIBRARIES ${LIBNXSERVER_LIBRARY} ${LIBNEXUS_LIBRARY} ${LIBNXCLIENT_LIBRARY})
set(LIBNXSERVER_INCLUDE_DIRS ${LIBNXSERVER_INCLUDE_DIR} ${LIBNXSERVER_INCLUDE_DIR} ${LIBNXIR_INCLUDE_DIR} ${NXREFSW_INCLUDE_PATH})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LIBNXSERVER DEFAULT_MSG LIBNXSERVER_LIBRARIES LIBNXSERVER_INCLUDE_DIRS)

mark_as_advanced(LIBNXSERVER_INCLUDE_DIRS LIBNXSERVER_LIBRARIES)
