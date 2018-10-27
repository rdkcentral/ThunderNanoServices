set(PLUGIN_NAME IOConnector)
set(MODULE_NAME WPEFramework${PLUGIN_NAME})


include(module) # Setup default stuff needed for the cmake framework thingy.

find_package(WPEFramework QUIET)

set(PLUGIN_INCLUDE_DIRS
    ${WPEFRAMEWORK_INCLUDE_DIRS})

set(PLUGIN_DEPENDENCIES ${SYSTEMD_LIBRARIES})

set(PLUGIN_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkPlugins}
    ${PLUGIN_DEPENDENCIES})

set(PLUGIN_SOURCES
    Module.cpp
    IOConnector.cpp
    GPIO.cpp
)

set(PLUGIN_INPUT
    ${PLUGIN_SOURCES}
    ${PLUGIN_INCLUDES}
    )

include(default_targets) # This is a regular plugin no need to edit the defaults

write_config( ${PLUGIN_NAME} )
