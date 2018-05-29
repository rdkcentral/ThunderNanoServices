set(PLUGIN_NAME NetworkControl)

include(module) # Setup default stuff needed for the cmake framework thingy.

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")

set(PLUGIN_DEPENDENCIES_INCLUDE_DIRS)

set(PLUGIN_DEFINITIONS
    PLUGIN_NETWORKCONTROL)

set(PLUGIN_INCLUDE_DIRS
    ${WPEFRAMEWORK_INCLUDE_DIRS}
    ${PLUGIN_DEPENDENCIES_INCLUDE_DIRS})

set(PLUGIN_OPTIONS)

set(PLUGIN_DEPENDENCIES)

set(PLUGIN_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkPlugins}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkCore}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkTracing}
    ${PLUGIN_DEPENDENCIES})

set(PLUGIN_SOURCES
    Module.cpp
    NetworkControl.cpp
    DHCPClientImplementation.cpp
)

set(PLUGIN_INPUT
    ${PLUGIN_SOURCES}
    ${PLUGIN_INCLUDES})

include(default_targets) # This is a regular plugin no need to edit the defaults

write_config( ${PLUGIN_NAME} )
