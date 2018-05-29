set(PLUGIN_NAME LocationSync)

include(module) # Setup default stuff needed for the cmake framework thingy.

set(WPEFRAMEWORK_PLUGIN_LOCATIONSYNC_URI location.webplatformforembedded.org CACHE STRING "URI to request the location information")

set(PLUGIN_DEFINITIONS)

set(PLUGIN_INCLUDE_DIRS
    ${WPEFRAMEWORK_INCLUDE_DIRS})

set(PLUGIN_OPTIONS)

set(PLUGIN_LINK_OPTIONS)

set(PLUGIN_DEPENDENCIES)

set(PLUGIN_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkPlugins}
    ${PLUGIN_DEPENDENCIES})

set(PLUGIN_SOURCES
    Module.cpp
    LocationSync.cpp
    LocationService.cpp
)

set(PLUGIN_INPUT
    ${PLUGIN_SOURCES}
    ${PLUGIN_INCLUDES}
    )

include(default_targets) # This is a regular plugin no need to edit the defaults

write_config( ${PLUGIN_NAME} )