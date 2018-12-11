if (PLUGIN_NAME STREQUAL "")
    message(FATAL_ERROR "unexpected: 'PLUGIN_NAME' seems not configured in your plugin's CMakeList.txt.")
endif()

set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

message("Setting up ${MODULE_NAME}")
