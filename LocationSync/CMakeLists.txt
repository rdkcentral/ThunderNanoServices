set(PLUGIN_NAME LocationSync)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(PLUGIN_LOCATIONSYNC_URI location.webplatformforembedded.org CACHE STRING "URI to request the location information")

set(PLUGIN_SOURCES
    Module.cpp
    LocationSync.cpp
    LocationService.cpp
)

# Library definition section
add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES})
target_link_libraries(${MODULE_NAME} ${PLUGINS_LIBRARIES})

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config( ${PLUGIN_NAME} )
