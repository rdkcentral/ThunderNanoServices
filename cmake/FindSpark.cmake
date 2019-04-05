# - Try to find Spark
# Once done this will define
#  SPARK_FOUND - System has LibSpark
#  SPARK_INCLUDE_DIRS - The libSpark include directories
#  SPARK_LIBRARIES - The libraries needed to use libSpark
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
pkg_check_modules(SPARK Spark)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Spark DEFAULT_MSG SPARK_LIBRARIES)

mark_as_advanced(SPARK_INCLUDE_DIRS SPARK_LIBRARIES)


find_library(SPARK_LIBRARY NAMES ${SPARK_LIBRARIES}
        HINTS ${SPARK_LIBDIR} ${SPARK_LIBRARY_DIRS}
        )

if(SPARK_LIBRARY AND NOT TARGET Spark::Spark)
    add_library(Spark::Spark UNKNOWN IMPORTED)
    set_target_properties(Spark::Spark PROPERTIES
            IMPORTED_LOCATION "${SPARK_LIBRARY}"
            INTERFACE_LINK_LIBRARIES "${SPARK_LIBRARIES}"
            INTERFACE_COMPILE_OPTIONS "${SPARK_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${SPARK_INCLUDE_DIRS}"
            )
endif()
