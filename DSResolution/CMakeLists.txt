set(PLUGIN_NAME DSResolution)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

message("Setting up ${PLUGIN_NAME}")

find_package(DSHAL REQUIRED)

set(PLUGIN_DSRESOLUTION_AUTOSTART false CACHE STRING "Automatically start DSResolution Plug-in")

set(PLUGIN_SOURCES
    DSResolution.cpp
    DSResolutionHAL.cpp
    Module.cpp)

# Library definition section
add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES})
target_link_libraries(${MODULE_NAME} ${PLUGINS_LIBRARIES} ${DSHAL_LIB})

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

if(DSRESOLUTION_WITH_DUMMY_DSHAL)
add_subdirectory(src)
endif(DSRESOLUTION_WITH_DUMMY_DSHAL)

write_config(${PLUGIN_NAME})
