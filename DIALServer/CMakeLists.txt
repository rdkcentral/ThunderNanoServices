set(PLUGIN_NAME DIALServer)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

option(PLUGIN_DIALSERVER_ENABLE_YOUTUBE "Enable YouTube support for DIAL server" OFF)
option(PLUGIN_DIALSERVER_ENABLE_NETFLIX "Enable Netflix support for DIAL server" OFF)

set(YOUTUBE_MODE "passive" CACHE STRING "How the DIAL server should process incomming requests from Youtube (passive/active), leave empty to disable")
set(NETFLIX_MODE "passive" CACHE STRING "How the DIAL server should process incomming requests from Netflix (passive/active), leave empty to disable")

set(WEBSERVER_CALLSIGN "" CACHE STRING "Callsign of the WebServer plugin if used, leave empty to disable")
set(SWITCHBOARD_CALLSIGN "" CACHE STRING "Callsign of the SwitchBoard plugin if used, leave empty to disable")

set(PLUGIN_DIALSERVER_NAME "${MODULE_NAME}" CACHE STRING "DIAL server name")

add_library(${MODULE_NAME} SHARED
    DIALServer.cpp
    DIALServerJsonRpc.cpp
    Module.cpp)

if(PLUGIN_DIALSERVER_ENABLE_YOUTUBE)
    target_sources(${MODULE_NAME}
        PRIVATE
            YouTube.cpp)
else()
    set(YOUTUBE_MODE "") # Disable the youtube entry in the config
endif()

if(PLUGIN_DIALSERVER_ENABLE_NETFLIX)
    target_sources(${MODULE_NAME}
        PRIVATE
            Netflix.cpp)
else()
    set(NETFLIX_MODE "") # Disable the netflix entry in the config
endif()

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

install(TARGETS ${MODULE_NAME}
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
