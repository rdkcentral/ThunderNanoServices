option(PLUGIN_WEBPA_CCSP "Enable WebPA CCSP client" OFF)
option(PLUGIN_WEBPA_GENERIC_ADAPTER "Enable WebPA generic client" OFF)

if (WEPPA_PLUGIN_CLIENT_CCSP)
    add_subdirectory(CCSP)
endif (WEPPA_PLUGIN_CLIENT_CCSP)

if (PLUGIN_WEBPA_GENERIC_ADAPTER)
    add_subdirectory(GenericAdapter)
endif (PLUGIN_WEBPA_GENERIC_ADAPTER)
