set(PLUGIN_NAME DIALServer)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)

option(PLUGIN_DIALSERVER_ENABLE_YOUTUBE "Enable YouTube support for DIAL server" OFF)
option(PLUGIN_DIALSERVER_ENABLE_NETFLIX "Enable Netflix support for DIAL server" OFF)
set(PLUGIN_DIALSERVER_NAME "${MODULE_NAME}" CACHE STRING "DIAL server name")

add_library(${MODULE_NAME} SHARED
    DIALServer.cpp
    DIALServerJsonRpc.cpp
    Module.cpp)

if(PLUGIN_DIALSERVER_ENABLE_YOUTUBE)
    target_sources(${MODULE_NAME}
        PRIVATE
            YouTube.cpp)
endif()

if(PLUGIN_DIALSERVER_ENABLE_NETFLIX)
    target_sources(${MODULE_NAME}
        PRIVATE
            Netflix.cpp)
endif()

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

install(TARGETS ${MODULE_NAME}
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
