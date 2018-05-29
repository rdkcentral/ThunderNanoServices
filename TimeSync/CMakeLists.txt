# WPEFRAMEWORK_ROOT acts as the root directory for this project, in case it is embedded in a larger, surrounding project.
# COMPONENT_NAME is used to group multiple install targets (one per module).
# MODULE_NAME is used as a replacement for PROJECT_NAME, as using project(x) and PROJECT_NAME seems to interfere with
# Yocto builds. In some cases where PROJECT_NAME is defined, we simple assign this variable to MODULE_NAME.
# Be careful to correctly set MODULE_NAME, as it is also used in the uninstall.cmake.in files for naming the uninstall manifest.
# Additionally, install targets are linked to the MODULE_NAME and COMPONENT_NAME.

set(PLUGIN_NAME TimeSync)

include(module) # Setup default stuff needed for the cmake framework.

set(PLUGIN_DEFINITIONS
    )

set(PLUGIN_INCLUDE_DIRS
    ${WPEFRAMEWORK_INCLUDE_DIRS})

set(PLUGIN_OPTIONS)

set(PLUGIN_LINK_OPTIONS)

set(PLUGIN_DEPENDENCIES
    )

set(PLUGIN_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkPlugins}
    ${PLUGIN_DEPENDENCIES})

file(GLOB PLUGIN_INCLUDES *.h)

set(PLUGIN_SOURCES
    Module.cpp
    TimeSync.cpp
    NTPClient.cpp
)

set(PLUGIN_INPUT
    ${PLUGIN_SOURCES}
    ${PLUGIN_INCLUDES}
    )

include(default_targets) # This is a regular plugin no need to edit the defaults

write_config(${PLUGIN_NAME})
