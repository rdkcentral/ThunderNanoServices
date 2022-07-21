# - Try to find SysCfg
# Once done this will define
#  SYSCFG_FOUND - System has SysCfg
#  SYSCFG_INCLUDE_DIRS - The SysCfg include directories
#  SYSCFG_LIBRARIES - The libraries needed to use SysCfg
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

# SysCfg has no pc file to search for

find_library(SYSCFG_LIBRARY syscfg)

if(EXISTS "${SYSCFG_LIBRARY}")
    include(FindPackageHandleStandardArgs)

    set(SYSCFG_FOUND TRUE)

    find_package_handle_standard_args(SYSCFG DEFAULT_MSG SYSCFG_FOUND SYSCFG_LIBRARY)
    mark_as_advanced(SYSCFG_LIBRARY)

    message("SYSCFG_LIBRARY = " ${SYSCFG_LIBRARY})
    if(NOT TARGET SysCfg::SysCfg)
        add_library(SysCfg::SysCfg UNKNOWN IMPORTED)
        set_target_properties(SysCfg::SysCfg PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${SYSCFG_LIBRARY}"
                )
    endif()
else()
    if(SysCfg_FIND_REQUIRED)
        message(FATAL_ERROR "SYSCFG_LIBRARY not available")
    elseif(NOT SysCfg_FIND_QUIETLY)
        message(STATUS "SYSCFG_LIBRARY not available")
    endif()
endif()
