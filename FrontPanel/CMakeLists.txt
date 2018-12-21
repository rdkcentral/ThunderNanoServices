set(PLUGIN_NAME FrontPanel)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

message("Setting up ${PLUGIN_NAME}")

find_package(DSHAL REQUIRED)

set(PLUGIN_FRONTPANEL_AUTOSTART false CACHE STRING "Automatically start FrontPanel plugin")

set(PLUGIN_SOURCES
    FrontPanel.cpp
    FrontPanelHAL.cpp
    Module.cpp)

# Library definition section
add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES})
target_link_libraries(${MODULE_NAME} ${PLUGINS_LIBRARIES} ${DSHAL_LIB})

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config(${PLUGIN_NAME})
