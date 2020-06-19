# Copyright RDK Management
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation, version 2
# of the license.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
# Boston, MA 02110-1301, USA.

find_package(${NAMESPACE}Definitions REQUIRED)

set(DOLBY_IMPLEMENTATION "NONE" CACHE STRING "Dolby C API implementation type." )

add_library(PlayerInfoDolby STATIC "")

target_include_directories(PlayerInfoDolby
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(${DOLBY_IMPLEMENTATION} STREQUAL "AMLogic")

    find_library(DOLBY_LIBRARY
        NAMES dolbyms12)

    target_sources(PlayerInfoDolby
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/AMLogic/Implementation.cpp)

    target_link_libraries(PlayerInfoDolby
        PRIVATE
            ${DOLBY_LIBRARY}
            ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

else()
    message(SEND_ERROR "No DOLBY_IMPLEMENTATION picked. Turn off dolby support or pick one.")
endif()


