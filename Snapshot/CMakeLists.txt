set(PLUGIN_NAME Snapshot)

include(module) # Setup default stuff needed for the cmake framework thingy.

find_package(PNG REQUIRED)

set(PLUGIN_DEPENDENCIES_INCLUDE_DIRS
    ${PNG_INCLUDE_DIRS})

set(PLUGIN_OPTIONS)

set(PLUGIN_LINK_OPTIONS)

set(PLUGIN_DEPENDENCIES
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkCore}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkTracing}
    ${WPEFRAMEWORK_PLUGIN_COMMANDER_DEPENDENCIES}
    ${PNG_LIBRARIES}
    )

set(PLUGIN_SOURCES
    Module.cpp
    Snapshot.cpp
    )

add_subdirectory(Device)

include(default_targets) # This is a regular plugin no need to edit the defaults

write_config(${PLUGIN_NAME})
