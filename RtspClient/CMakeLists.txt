set(PLUGIN_NAME RtspClient)

set(WPEFRAMEWORK_PLUGIN_SAMPLE_AUTOSTART true CACHE STRING true)
set(WPEFRAMEWORK_PLUGIN_SAMPLE_OOP false CACHE STRING true)

include(module) # Setup default stuff needed for the cmake framework thingy.

set(PLUGIN_OPTIONS)

set(PLUGIN_SOURCES
        Module.cpp
        RtspClient.cpp
        RtspClientImplementation.cpp
        RtspParser.cpp
        RtspSocket.cpp
        RtspParser.cpp
        RtspSession.cpp
        RtspSessionInfo.cpp
        )

include(default_targets) # This is a regular plugin no need to edit the defaults

if (WPEFRAMEWORK_VERBOSE_BUILD)
    display_list("Defines                     : " ${PLUGIN_DEFINITIONS} )
    display_list("Compiler options            : " ${PLUGIN_OPTIONS} )
    display_list("Source files                : " ${PLUGIN_SOURCES} )
    display_list("Include files               : " ${PLUGIN_INCLUDES} )
    display_list("Include dirs                : " ${PLUGIN_INCLUDE_DIRS} )
    display_list("Link libs                   : " ${PLUGIN_LIBS} )
    display_list("Linker options              : " ${PLUGIN_LINK_OPTIONS} )
    display_list("Dependencies                : " ${PLUGIN_DEPENDENCIES} )
endif()


write_config(${PLUGIN_NAME})
