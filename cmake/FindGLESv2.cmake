# - Try to Find GLESv2
# Once done, this will define
#
#  GLESV2_FOUND - system has EGL installed.
#  GLESV2_INCLUDE_DIRS - directories which contain the EGL headers.
#  GLESV2_LIBRARIES - libraries required to link against EGL.
#  GLESV2_DEFINITIONS - Compiler switches required for using EGL.
#

find_package(PkgConfig)

pkg_check_modules(PC_GLESV2 glesv2)

if (PC_GLESV2_FOUND)
    set(GLESV2_CFLAGS ${PC_GLESV2_CFLAGS})
    set(GLESV2_FOUND ${PC_GLESV2_FOUND})
    set(GLESV2_DEFINITIONS ${PC_GLESV2_CFLAGS_OTHER})
    set(GLESV2_NAMES ${PC_GLESV2_LIBRARIES})
    foreach (_library ${GLESV2_NAMES})
        find_library(GLESV2_LIBRARIES_${_library} ${_library}
	    HINTS ${PC_GLESV2_LIBDIR} ${PC_GLESV2_LIBRARY_DIRS}
        )
        set(GLESV2_LIBRARIES ${GLESV2_LIBRARIES} ${GLESV2_LIBRARIES_${_library}})
    endforeach ()
    set(GLESV2_INCLUDE_DIRS ${PC_GLESV2_INCLUDE_DIRS} CACHE FILEPATH "FIXME")
endif ()

find_library(GLESV2_LIBRARY NAMES GLESv2
        HINTS ${PC_EGL_LIBDIR} ${PC_EGL_LIBRARY_DIRS}
        )

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLESV2 DEFAULT_MSG GLESV2_FOUND)

if(GLESV2_FOUND AND NOT TARGET EGL::GLESV2)
    add_library(EGL::GLESV2 UNKNOWN IMPORTED)
    set_target_properties(EGL::GLESV2 PROPERTIES
            IMPORTED_LOCATION "${GLESV2_LIBRARY}"
            INTERFACE_LINK_LIBRARIES "${GLESV2_LIBRARIES}"
            INTERFACE_COMPILE_OPTIONS "${GLESV2_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${GLESV2_INCLUDE_DIRS}"
            )
endif()


mark_as_advanced(GLESV2_INCLUDE_DIRS GLESV2_LIBRARIES)
