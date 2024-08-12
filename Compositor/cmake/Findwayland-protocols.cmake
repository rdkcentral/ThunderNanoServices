# - Try to find Weston Compositor library.
# Once done this will define
#
# Copyright (C) 2023 Metrological.
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

if(wayland-protocols_FIND_QUIETLY)
    set(FIND_MODE QUIET)
elseif(wayland-protocols_FIND_REQUIRED)
    set(FIND_MODE REQUIRED)
endif()

find_package(PkgConfig)

pkg_check_modules(wayland-protocols IMPORTED_TARGET ${FIND_MODE} wayland-protocols>=1.13 )

if(wayland-protocols_FOUND)

# find Wayland protocols
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

# find 'wayland-scanner' executable
pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)

message(STATUS "WAYLAND_PROTOCOLS_DIR: ${WAYLAND_PROTOCOLS_DIR}")
message(STATUS "WAYLAND_SCANNER: ${WAYLAND_SCANNER}")

function(WaylandGenerator)
    set(optionsArgs SKIP_CLIENT_HEADER SKIP_PRIVATE_CODE )
    set(oneValueArgs SOURCES HEADERS)
    set(multiValueArgs DEFENITIONS)

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(Argument_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to WaylandGenerator(): \"${Arg_UNPARSED_ARGUMENTS}\".")
    endif()

    foreach(DEFENITION ${Argument_DEFENITIONS})
        if(EXISTS ${DEFENITION})
            set(DEFENITION_FILE_PATH ${DEFENITION})

            get_filename_component(DEFENITION_FILE_PATH ${DEFENITION} REALPATH)
            get_filename_component(DEFENITION ${DEFENITION_FILE_PATH} NAME_WE)
            message(VERBOSE "Found ${DEFENITION_FILE_PATH}")
        else()
            string(REGEX REPLACE "-" ";" DEFENITION_LIST "${DEFENITION}")

            set(SUFFIXES "")
            set(SUB_DEFENITION "")
            set(DEFENITION_FILE "${DEFENITION}.xml")

            foreach(DEFENITION ${DEFENITION_LIST})
                list(APPEND SUB_DEFENITION ${DEFENITION} )
                list(JOIN SUB_DEFENITION "-" JOINED)
                list(APPEND SUFFIXES ${JOINED} ) 
            endforeach()

            find_file(${DEFENITION}_FILE_PATH "${DEFENITION_FILE}"
            NO_DEFAULT_PATH
            PATHS 
                    "${WAYLAND_PROTOCOLS_DIR}"
                    "${WAYLAND_PROTOCOLS_DIR}/stable"  
                    "${WAYLAND_PROTOCOLS_DIR}/staging"  
                    "${WAYLAND_PROTOCOLS_DIR}/unstable"
            PATH_SUFFIXES 
                    ${SUFFIXES}
            )

            get_filename_component(DEFENITION_FILE_PATH ${${DEFENITION}_FILE_PATH} REALPATH)

            if(NOT EXISTS ${DEFENITION_FILE_PATH})
                message(FATAL_ERROR "could not find ${DEFENITION}.xml")
            else()
                message(VERBOSE "Found ${DEFENITION_FILE_PATH}")
            endif()
        endif()

        if(NOT ${Argument_SKIP_CLIENT_HEADER})
            message(VERBOSE "Generating ${DEFENITION}-client-protocol.h")

            list(APPEND HEADERS "${CMAKE_CURRENT_BINARY_DIR}/generated/${DEFENITION}-protocol.c")

            add_custom_command(
                    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/${DEFENITION}-client-protocol.h
                    COMMAND ${WAYLAND_SCANNER} client-header "${DEFENITION_FILE_PATH}" ${CMAKE_CURRENT_BINARY_DIR}/generated/${DEFENITION}-client-protocol.h)
        endif()

        if(NOT ${Argument_SKIP_PRIVATE_CODE})
            message(VERBOSE "Generating ${DEFENITION}-protocol.c")

            list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/generated/${DEFENITION}-protocol.c")

            add_custom_command(
                    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/${DEFENITION}-protocol.c
                    COMMAND ${WAYLAND_SCANNER} private-code "${DEFENITION_FILE_PATH}" ${CMAKE_CURRENT_BINARY_DIR}/generated/${DEFENITION}-protocol.c
                    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/generated/${DEFENITION}-client-protocol.h)
        endif()
    endforeach()

    if(NOT "${Argument_SOURCES}" STREQUAL "")
        set(${Argument_SOURCES} ${SOURCES} PARENT_SCOPE )
    endif()

    if(NOT "${Argument_HEADERS}" STREQUAL "")
        set(${Argument_HEADERS} ${HEADERS} PARENT_SCOPE)
    endif()

endfunction(WaylandGenerator)


add_library(wayland-protocols::wayland-protocols ALIAS PkgConfig::wayland-protocols)
endif()
