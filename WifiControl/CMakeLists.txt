set(PLUGIN_NAME WifiControl)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

message("Setting up ${PLUGIN_NAME}")

set(PLUGIN_SOURCES
    Controller.cpp  
    Network.cpp      
    Module.cpp)

if (PLUGIN_USE_RDK_HAL_WIFI)
    target_compile_definitions(${MODULE_NAME} USE_WIFI_HAL)
    list(APPEND PLUGIN_SOURCES WifiHAL.cpp)
else ()
    list(APPEND PLUGIN_SOURCES Controller.cpp)
endif()

# Library definition section
add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES})
target_link_libraries(${MODULE_NAME} ${PLUGINS_LIBRARIES})

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config(${PLUGIN_NAME})
