# WPEFRAMEWORK_ROOT acts as the root directory for this project, in case it is embedded in a larger, surrounding project.
# COMPONENT_NAME is used to group multiple install targets (one per module).
# MODULE_NAME is used as a replacement for PROJECT_NAME, as using project(x) and PROJECT_NAME seems to interfere with
# Yocto builds. In some cases where PROJECT_NAME is defined, we simple assign this variable to MODULE_NAME.
# Be careful to correctly set MODULE_NAME, as it is also used in the uninstall.cmake.in files for naming the uninstall manifest.
# Additionally, install targets are linked to the MODULE_NAME and COMPONENT_NAME.
set(PLUGIN_NAME Streamer)

include(module) # Setup default stuff needed for the cmake framework.

find_package(Interfaces REQUIRED)

set(PLUGIN_DEFINITIONS
    )

set(PLUGIN_OPTIONS)

set(PLUGIN_LINK_OPTIONS)

set(PLUGIN_INCLUDE_DIRS
    ${PLUGINS_INCLUDE_DIR})

file(GLOB PLUGIN_INCLUDES *.h)

set(PLUGIN_SOURCES
    Module.cpp
    Frontend.cpp
    Streamer.cpp
    StreamerImplementation.cpp
)

set(PLUGIN_INPUT
    ${PLUGIN_SOURCES}
    ${PLUGIN_INCLUDES}
    )

option(STREAMER_IMPLEMENTATION "Define the actual implementation to be used for this player" Stub)
add_subdirectory(Implementation/${STREAMER_IMPLEMENTATION})
include_directories(Implementation/${STREAMER_IMPLEMENTATION})
set(STREAMER_LIBS -Wl,--whole-archive PlayerPlatform -Wl,--no-whole-archive)

set(PLUGIN_DEPENDENCIES
    ${PLUGINS_LIBRARIES}
    ${ADDITIONAL_LIBRARIES}
    ${STREAMER_LIBS}
    ${INTERFACES_LIBRARIES})

include(default_targets) # This is a regular plugin no need to edit the defaults

write_config(${PLUGIN_NAME})
