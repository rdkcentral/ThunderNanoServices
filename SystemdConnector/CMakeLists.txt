set(PLUGIN_NAME SystemdConnector)
set(MODULE_NAME WPEFramework${PLUGIN_NAME})

find_package(Systemd REQUIRED)

set(PLUGIN_SOURCES
    SystemdConnector.cpp
    Module.cpp)

# Library definition section
add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES})
target_link_libraries(${MODULE_NAME} ${PLUGINS_LIBRARIES} ${SYSTEMD_LIBRARIES})

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config( ${PLUGIN_NAME} )
