set(PLUGIN_NAME DSResolution)
set(MODULE_NAME WPEFramework${PLUGIN_NAME})

set(WPEFRAMEWORK_PLUGIN_DSRESOLUTION_AUTOSTART false CACHE STRING "Automatically start DSResolution Plug-in")

find_package(DSHAL REQUIRED)

include(module)  # Setup default stuff needed for the cmake framework thingy.

set(PLUGIN_OPTIONS)

set(PLUGIN_DEPENDENCIES
    ${DSHAL_LIB}
)
set(PLUGIN_INCLUDE_DIRS ${WPEFRAMEWORK_INCLUDE_DIRS})

set(PLUGIN_SOURCES
    Module.cpp
    DSResolution.cpp
    DSResolutionHAL.cpp
)

set(PLUGIN_LIBS1
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    ${DSHAL_LIB}
    ${PLUGIN_DEPENDENCIES})

include(default_targets)  # This is a regular plugin no need to edit the defaults

#if (WPEFRAMEWORK_VERBOSE_BUILD)
    display_list("Defines                     : " ${PLUGIN_DEFINITIONS} )
    display_list("Compiler options            : " ${PLUGIN_OPTIONS} )
    display_list("Source files                : " ${PLUGIN_SOURCES} )
    display_list("Include files               : " ${PLUGIN_INCLUDES} )
    display_list("Include dirs                : " ${PLUGIN_INCLUDE_DIRS} )
    display_list("Link libs                   : " ${PLUGIN_DEPENDENCIES} )
    display_list("Linker options              : " ${PLUGIN_LINK_OPTIONS} )
    display_list("Dependencies                : " ${PLUGIN_DEPENDENCIES} )
#endif()

target_link_libraries(${MODULE_NAME} ${PLUGIN_LIBS1})
target_include_directories(${MODULE_NAME} PRIVATE ${PLUGIN_INCLUDE_DIRS})

if(DSRESOLUTION_WITH_DUMMY_DSHAL)
add_subdirectory(src)
endif(DSRESOLUTION_WITH_DUMMY_DSHAL)
write_config(${PLUGIN_NAME})
