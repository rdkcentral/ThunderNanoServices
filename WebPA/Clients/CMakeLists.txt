option(PLUGIN_WEBPA_CCSP "Enable WebPA CCSP client" OFF)
option(PLUGIN_WEBPA_GENERIC_ADAPTER "Enable WebPA generic client" OFF)

if (PLUGIN_WEBPA_CCSP)
    add_subdirectory(CCSP)
endif (PLUGIN_WEBPA_CCSP)

if (PLUGIN_WEBPA_GENERIC_ADAPTER)
    add_subdirectory(GenericAdapter)
endif (PLUGIN_WEBPA_GENERIC_ADAPTER)
