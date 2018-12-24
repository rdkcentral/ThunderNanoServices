set(PLUGIN_NAME RtspClient)

set(PLUGIN_RTSPCLIENT_AUTOSTART true CACHE STRING true)
set(PLUGIN_RTSPCLIENT_OOP true CACHE STRING true)

include(module) # Setup default stuff needed for the cmake framework thingy.

set(PLUGIN_OPTIONS)

set(PLUGIN_SOURCES
        Module.cpp
        RtspClient.cpp
        RtspClientImplementation.cpp
        RtspParser.cpp
        RtspParser.cpp
        RtspSession.cpp
        RtspSessionInfo.cpp
        )

include(default_targets) # This is a regular plugin no need to edit the defaults

write_config(${PLUGIN_NAME})
