set(PLUGIN_NAME Streamer)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

option(STREAMER_IMPLEMENTATION "Define the actual implementation to be used for this player" Stub)

find_package(${NAMESPACE}Definitions REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)

add_library(${MODULE_NAME} SHARED
    Module.cpp
    Frontend.cpp
    Streamer.cpp
    StreamerImplementation.cpp
    StreamerJsonRpc.cpp
)

add_subdirectory(Implementation/${STREAMER_IMPLEMENTATION})
include_directories(Implementation/${STREAMER_IMPLEMENTATION})

target_include_directories(${MODULE_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        PlayerPlatform)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
