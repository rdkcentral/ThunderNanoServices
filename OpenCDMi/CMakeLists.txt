set(PLUGIN_NAME OCDM)

set(WPEFRAMEWORK_PLUGIN_OPENCDMI_AUTOSTART true CACHE STRING true)
set(WPEFRAMEWORK_PLUGIN_OPENCDMI_OOP false CACHE STRING true)

include(module) # Setup default stuff needed for the cmake framework thingy.

set(PLUGIN_OPTIONS)
set(PLUGIN_LINK_OPTIONS -locdm)

set(PLUGIN_SOURCES 
        Module.cpp
        OCDM.cpp
        CENCParser.cpp
        FrameworkRPC.cpp)

include(default_targets) # This is a regular plugin no need to edit the defaults

write_config(${PLUGIN_NAME})
