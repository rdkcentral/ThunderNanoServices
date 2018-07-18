set(PLUGIN_NAME Compositor)

option(WPEFRAMEWORK_PLUGIN_COMPOSITOR_SYSTEM "Callsign to report information." "Controller")
option(WPEFRAMEWORK_PLUGIN_COMPOSITOR_IMPLEMENTATION "Defines which implementation is used." "None")
option(WPEFRAMEWORK_PLUGIN_COMPOSITOR_HARDWAREREADY "Manual overrule of the time it takes to initialisize all hardware (s) [0 - UINT16_MAX]." 0)
option(WPEFRAMEWORK_PLUGIN_COMPOSITOR_SERVICE "Defines where to get resouce handles. [internal, external]." external)
option(WPEFRAMEWORK_PLUGIN_COMPOSITOR_OUTOFPROCESS "Controls if the plugin should run in its own process. [true, false]." true)
option(WPEFRAMEWORK_PLUGIN_COMPOSITOR_AUTOTRACE "Contols if the plugin should automatically start tracing. [ON, OFF]." OFF)
option(WPEFRAMEWORK_PLUGIN_COMPOSITOR_BOXMODE "Allows for selecting a boxmode (Nexus only).")
option(WPEFRAMEWORK_PLUGIN_COMPOSITOR_GRAPHICS_HEAP_SIZE "Change graphic heap of driver (Nexus only).")

set(WPEFRAMEWORK_PLUGIN_COMPOSITOR_IMPLEMENTATION_LIB "libcompositorplugin.so" CACHE STRING "Specify a library with a compositor implentation." )

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION})

include(module)


set(PLUGIN_DEPENDENCIES_INCLUDE_DIRS)
set(PLUGIN_DEPENDENCIES
        ${WPEFRAMEWORK_PLUGIN_COMPOSITOR_CLIENT_LIBS}
        )
set(PLUGIN_DEFINITIONS)
set(PLUGIN_OPTIONS)
set(PLUGIN_LINK_OPTIONS)
set(PLUGIN_SOURCES
    Module.cpp
    Compositor.cpp
    )

include(default_targets)

if(WPEFRAMEWORK_PLUGIN_COMPOSITOR_SERVER)
    add_subdirectory(server)
endif ()

add_subdirectory(lib)

write_config(${PLUGIN_NAME})
