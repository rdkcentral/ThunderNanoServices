set(PLUGIN_NAME DsgccClient)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(Interfaces REQUIRED)

file(GLOB PLUGIN_INCLUDES *.h)

set(PLUGIN_SOURCES
        Module.cpp
        DsgccClient.cpp
        DsgccClientImplementation.cpp
        )

add_definitions(-DDSG_RPC_INCLUDED=1 -DBCM_DSGCC_CLIENTS_USE_SHARED_MEMORY_IPC=1 -DTARGETOS_Linux)
list(APPEND PLUGINS_LIBRARIES dsgclient)

message ("PLUGINS_LIBRARIES=${PLUGINS_LIBRARIES}")

# Library definition section
add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES})
target_link_libraries(${MODULE_NAME} ${PLUGINS_LIBRARIES} ${INTERFACES_LIBRARIES} ${ADDITIONAL_LIBRARIES})

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config(${PLUGIN_NAME})
