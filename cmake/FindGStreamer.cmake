# - Try to find gstreamer
# Once done this will define
#  GSTREAMER_FOUND - System has gstreamer
#  GSTREAMER_INCLUDE_DIRS - The gstreamer include directories
#  GSTREAMER_LIBRARIES - The libraries needed to use gstreamer

#gstreamer has no pc file to search for

find_path(
    GSTREAMER_INCLUDE_DIR
    NAMES gst/gst.h
    PATH_SUFFIXES gstreamer-1.0
    HINTS /usr/include /usr/local/include ${CMAKE_INSTALL_PREFIX}/usr/include)

find_library(
    GSTREAMER_LIBRARY
    NAMES gstreamer-1.0
    HINTS /usr/lib /usr/local/lib ${CMAKE_INSTALL_PREFIX}/usr/lib
    PATH_SUFFIXES gstreamer-1.0
)    

if("${GSTREAMER_INCLUDE_DIR}" STREQUAL "" OR "${GSTREAMER_LIBRARY}" STREQUAL "")
    set(GSTREAMER_FOUND_TEXT "Not found")
else()
    set(GSTREAMER_FOUND_TEXT "Found")
endif()

if (WPEFRAMEWORK_VERBOSE_BUILD)
    message(STATUS "gstreamer       : ${GSTREAMER_FOUND_TEXT}")
    message(STATUS "  version      : ${PC_GSTREAMER_VERSION}")
    message(STATUS "  cflags       : ${PC_GSTREAMER_CFLAGS}")
    message(STATUS "  cflags other : ${PC_GSTREAMER_CFLAGS_OTHER}")
    message(STATUS "  include dirs : ${GSTREAMER_INCLUDE_DIR}")
    message(STATUS "  libs         : ${GSTREAMER_LIBRARY}")
endif()

set(GSTREAMER_DEFINITIONS ${PC_GSTREAMER_PLUGINS_CFLAGS_OTHER})
set(GSTREAMER_INCLUDE_DIRS ${GSTREAMER_INCLUDE_DIR})
set(GSTREAMER_LIBRARIES ${GSTREAMER_LIBRARY})
set(GSTREAMER_LIBRARY_DIRS ${PC_GSTREAMER_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GSTREAMER DEFAULT_MSG
    GSTREAMER_LIBRARIES GSTREAMER_INCLUDE_DIRS)

if(GSTREAMER_FOUND)
    message(WARNING "got gstreamer!!!!!!!!!!!!!!!")
else()
    message(WARNING "Could not find gstreamer!!!!!!!!!!!!!!")
endif()

mark_as_advanced(GSTREAMER_DEFINITIONS GSTREAMER_INCLUDE_DIRS GSTREAMER_LIBRARIES)
