if (PLUGIN_NAME STREQUAL "")
    message(FATAL_ERROR "unexpected: 'PLUGIN_NAME' seems not configured in your plugin's CMakeList.txt.")
endif()

set(MODULE_NAME WPEFramework${PLUGIN_NAME})

message("Setting up ${MODULE_NAME}")

find_package(WPEFramework QUIET)

if(CLION_ENVIRONMENT)
    set(WPEFRAMEWORK_LIBRARY_WPEFrameworkPlugins WPEFrameworkPlugins)
    set(WPEFRAMEWORK_INCLUDE_DIRS ${WPEFRAMEWORK_ROOT}/Source)
endif()
