set(PLUGIN_NAME Compositor)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
set(PLATFORM_COMPOSITOR "compositorplugin")

option(PLUGIN_COMPOSITOR_SYSTEM "Callsign to report information." "Controller")
option(PLUGIN_COMPOSITOR_IMPLEMENTATION "Defines which implementation is used." "None")
option(PLUGIN_COMPOSITOR_HARDWAREREADY "Manual overrule of the time it takes to initialisize all hardware (s) [0 - UINT16_MAX]." 0)
option(PLUGIN_COMPOSITOR_SERVICE "Defines where to get resouce handles. [internal, external]." external)
option(PLUGIN_COMPOSITOR_OUTOFPROCESS "Controls if the plugin should run in its own process. [true, false]." true)
option(PLUGIN_COMPOSITOR_AUTOTRACE "Contols if the plugin should automatically start tracing. [ON, OFF]." OFF)
option(PLUGIN_COMPOSITOR_BOXMODE "Allows for selecting a boxmode (Nexus only).")
option(PLUGIN_COMPOSITOR_GRAPHICS_HEAP_SIZE "Change graphic heap of driver (Nexus only).")

set(PLUGIN_COMPOSITOR_IMPLEMENTATION_LIB "lib${PLATFORM_COMPOSITOR}.so" CACHE STRING "Specify a library with a compositor implentation." )
set(PLUGIN_COMPOSITOR_RESOLUTION "720p" CACHE STRING "Specify the startup resolution")

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION})

find_package(Interfaces REQUIRED)

set(PLUGIN_SOURCES
    Module.cpp
    Compositor.cpp)

file(GLOB PLUGIN_INCLUDES *.h)

add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES} ${PLUGIN_INCLUDES})
target_link_libraries(${MODULE_NAME} ${PLUGINS_LIBRARIES} ${INTERFACES_LIBRARIES} ${PLUGIN_COMPOSITOR_CLIENT_LIBS})
set_target_properties(${MODULE_NAME} PROPERTIES OUTPUT_NAME ${MODULE_NAME})

string(TOLOWER ${NAMESPACE} STORAGENAME)
install(
    TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins
    COMPONENT ${MODULE_NAME})

if(PLUGIN_COMPOSITOR_SERVER)
    add_subdirectory(server)
endif ()

add_subdirectory(lib)

write_config(${PLUGIN_NAME})
