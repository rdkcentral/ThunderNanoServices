set(PLUGIN_NAME IOConnector)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED 
    Module.cpp
    IOConnector.cpp
    IOConnectorJsonRpc.cpp
    GPIO.cpp
    Handler.cpp
)

if(PLUGIN_IOCONNECTOR_PAIRING_PIN)
    target_sources(${MODULE_NAME} PRIVATE RemotePairing.cpp)
endif(PLUGIN_IOCONNECTOR_PAIRING_PIN)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME} 
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

install(TARGETS ${MODULE_NAME} 
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
