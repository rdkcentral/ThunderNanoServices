set(PLUGIN_NAME Streamer)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

option(STREAMER_IMPLEMENTATION "Define the actual implementation to be used for this player" Stub)

find_package(${NAMESPACE}Interfaces REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)

add_library(${MODULE_NAME} SHARED 
    Module.cpp
    Frontend.cpp
    Streamer.cpp
    StreamerImplementation.cpp
)

add_subdirectory(Implementation/${STREAMER_IMPLEMENTATION})

target_link_libraries(${MODULE_NAME}
    PRIVATE 
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Interfaces::${NAMESPACE}Interfaces
        PlayerPlatform)

install(TARGETS ${MODULE_NAME} 
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config(${PLUGIN_NAME})
