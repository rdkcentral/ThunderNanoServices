set(PLUGIN_NAME IOConnector)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(PLUGIN_SOURCES
    Module.cpp
    IOConnector.cpp
    GPIO.cpp
    Handler.cpp
)

if(PLUGIN_IOCONNECTOR_PAIRING_PIN)
    list(APPEND PLUGIN_SOURCES RemotePairing.cpp)
endif(PLUGIN_IOCONNECTOR_PAIRING_PIN)

# Library definition section
add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES})
target_link_libraries(${MODULE_NAME} ${PLUGINS_LIBRARIES})

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config( ${PLUGIN_NAME} )
