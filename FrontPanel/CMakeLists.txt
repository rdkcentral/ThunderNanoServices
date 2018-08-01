set(PLUGIN_NAME FrontPanel)

find_package(DSHAL REQUIRED)

set(WPEFRAMEWORK_PLUGIN_FRONTPANEL_AUTOSTART false CACHE STRING "Automatically start FrontPanel plugin")

include(module) # Setup default stuff needed for the cmake framework thingy.

set(PLUGIN_OPTIONS)

set(PLUGIN_DEPENDENCIES
    ${DSHAL_LIBRARIES}
)    

set(PLUGIN_SOURCES 
    Module.cpp
    FrontPanel.cpp
    FrontPanelHAL.cpp
)

include(default_targets) # This is a regular plugin no need to edit the defaults

write_config(${PLUGIN_NAME})
