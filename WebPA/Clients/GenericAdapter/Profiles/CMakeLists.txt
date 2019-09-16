option(PLUGIN_WEBPA_DEVICE_INFO "Include Device Interface Control" OFF)

if(PLUGIN_WEBPA_DEVICE_INFO)
    add_subdirectory (Device)
endif(PLUGIN_WEBPA_DEVICE_INFO)
