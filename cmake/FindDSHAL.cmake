# - Try to find dvb-apps
# Once done this will define
#  DSHAL_FOUND - System has dvb-apps
#  DSHAL_INCLUDE_DIRS - The dvb-apps include directories
#  DSHAL_LIBRARIES - The libraries needed to use dvb-apps

#dvb-apps has no pc file to search for

find_path(
    DSHAL_INCLUDE_DIR
    NAMES dsFPD.h
    PATH_SUFFIXES libds-hal
)

find_library(
    DSHAL_LIBRARY
    NAMES ds-hal
    HINTS /usr/lib /usr/local/lib ${CMAKE_INSTALL_PREFIX}/usr/lib
)

if("${DSHAL_INCLUDE_DIR}" STREQUAL "" OR "${DSHAL_LIBRARY}" STREQUAL "")
    set(DSHAL_FOUND_TEXT "Not found")
    else()
    set(DSHAL_FOUND_TEXT "Found")
endif()

if (WPEFRAMEWORK_VERBOSE_BUILD)
    message(STATUS "dvb-apps       : ${DSHAL_FOUND_TEXT}")
    message(STATUS "  version      : ${PC_DSHAL_VERSION}")
    message(STATUS "  cflags       : ${PC_DSHAL_CFLAGS}")
    message(STATUS "  cflags other : ${PC_DSHAL_CFLAGS_OTHER}")
    message(STATUS "  include dirs : ${DSHAL_INCLUDE_DIR}")
    message(STATUS "  libs         : ${DSHAL_LIBRARY}")
endif()



set(DSHAL_INCLUDE_DIRS ${DSHAL_INCLUDE_DIR})
set(DSHAL_LIBRARIES ${DSHAL_LIBRARY})
set(DSHAL_LIBRARY_DIRS ${PC_DSHAL_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DSHAL DEFAULT_MSG
    DSHAL_LIBRARIES DSHAL_INCLUDE_DIRS)

mark_as_advanced(DSHAL_DEFINITIONS DSHAL_INCLUDE_DIRS DSHAL_LIBRARIES)
