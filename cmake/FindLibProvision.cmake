# - Try to find LibProvision
# Once done this will define
#  LIBPROVISION_FOUND - System has LibProvision
#  LIBPROVISION_INCLUDE_DIRS - The LibProvision include directories
#  LIBPROVISION_LIBRARIES - The libraries needed to use LibProvision
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

find_package(PkgConfig)
pkg_check_modules(PC_PROVISION QUIET provision)

find_path(LIBPROVISION_INCLUDE_DIR Provision.h
           PATHS usr/include
           PATH_SUFFIXES provision)

find_library(LIBPROVISION_LIBRARY NAMES provision
             HINTS ${PC_PROVISION_LIBDIR} ${PC_PROVISION_LIBRARY_DIRS})
             
set(LIBPROVISION_LIBRARIES ${LIBPROVISION_LIBRARY})
set(LIBPROVISION_INCLUDE_DIRS ${LIBPROVISION_INCLUDE_DIR} ${LIBRPC_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LIBPROVISION DEFAULT_MSG LIBPROVISION_LIBRARIES LIBPROVISION_INCLUDE_DIRS)

mark_as_advanced(LIBPROVISION_INCLUDE_DIRS LIBPROVISION_LIBRARIES )
