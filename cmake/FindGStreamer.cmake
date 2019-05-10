# - Try to find gstreamer
# Once done this will define
#  GSTREAMER_FOUND - System has gstreamer
#  GSTREAMER::GSTREAMER - The GStreamer library

#gstreamer has no pc file to search for

find_path(GSTREAMER_INCLUDE gst/gst.h
    PATH_SUFFIXES gstreamer-1.0
    HINTS /usr/include /usr/local/include ${CMAKE_INSTALL_PREFIX}/usr/include)

find_library(GSTREAMER_LIBRARY NAMES gstreamer-1.0)

if(EXISTS "${GSTREAMER_LIBRARY}")
    include(FindPackageHandleStandardArgs)

    set(GSTREAMER_FOUND TRUE)

    find_package_handle_standard_args(GSTREAMER_LIBRARY DEFAULT_MSG GSTREAMER_FOUND GSTREAMER_INCLUDE GSTREAMER_LIBRARY)
    mark_as_advanced(GSTREAMER_INCLUDE GSTREAMER_LIBRARY)

    if(NOT TARGET GSTREAMER::GSTREAMER)
        add_library(GSTREAMER::GSTREAMER UNKNOWN IMPORTED)

        set_target_properties(GSTREAMER::GSTREAMER PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${GSTREAMER_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${GSTREAMER_INCLUDE}"
        )
    endif()
endif()
