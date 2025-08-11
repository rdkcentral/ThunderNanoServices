# - Try to Find libdecor
# Once done, this will define
#
#  LIBDECOR_FOUND - system has LIBDECOR installed.
#  LIBDECOR_INCLUDE_DIRS - directories which contain the LIBDECOR headers.
#  LIBDECOR_LIBRARIES - libraries required to link against LIBDECOR.
#  LIBDECOR_DEFINITIONS - Compiler switches required for using LIBDECOR.
#
# Copyright (C) 2012 Intel Corporation. All rights reserved.
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
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NLIBDECORIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

if(LIBDECOR_FIND_QUIETLY)
    set(_LIBDECOR_MODE QUIET)
elseif(LIBDECOR_FIND_REQUIRED)
    set(_LIBDECOR_MODE REQUIRED)
endif()

find_package(PkgConfig)
pkg_check_modules(libdecor ${_LIBDECOR_MODE} IMPORTED_TARGET GLOBAL libdecor-0)

if (libdecor_FOUND)
   add_library(libdecor::libdecor ALIAS PkgConfig::libdecor)
endif()