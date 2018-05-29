# - Try to find openCDMi
# Once done this will define
#  OPENCDMI_FOUND - System has libocdmi
#  OPENCDMI_INCLUDE_DIRS - The libocdmi include directories
#  OPENCDMI_LIBRARIES - The libraries needed to use libcdmi
#
# Copyright (C) 2016 TATA ELXSI
# Copyright (C) 2016 Metrological.
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
#

find_path (OPENCDMXDRSVC_INCLUDE_DIR NAME opencdm_xdr_svc.h PATH_SUFFIXES opencdmi)

find_library(OCDMI_LIB NAME libocdmi.so PATH_SUFFIXES lib)
       set (OPENCDMI_INCLUDE_DIRS ${OPENCDMXDRSVC_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
set (OPENCDMI_INCLUDE_DIRS ${OPENCDMXDRSVC_INCLUDE_DIR} CACHE PATH "Path to header")
set (OPENCDMI_LIBRARIES ${OCDMI_LIB} CACHE PATH "path to ocdmi library")
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OPENCDMI DEFAULT_MSG OPENCDMI_INCLUDE_DIRS OPENCDMI_LIBRARIES)

mark_as_advanced(OPENCDMI_INCLUDE_DIRS OPENCDMI_LIBRARIES)
