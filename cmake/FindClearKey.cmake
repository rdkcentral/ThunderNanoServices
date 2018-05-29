# Once done this will define
#  CLEARKEY_FOUND - System has libocdmi
#  CLEARKEY_INCLUDE_DIRS - The libocdmi include directories
#  CLEARKEY_LIBRARIES - The libraries needed to use libcdmi
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

find_path (CLEARKEY_INCLUDE_DIRS NAME "opensslconf.h" PATHS "usr/include/" PATH_SUFFIXES "openssl" )
find_library(CLEARKEY_LIBRARIES NAME crypto PATH_SUFFIXES lib)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CLEARKEY DEFAULT_MSG CLEARKEY_INCLUDE_DIRS CLEARKEY_LIBRARIES)

mark_as_advanced(CLEARKEY_INCLUDE_DIRS CLEARKEY_LIBRARIES)
