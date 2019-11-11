set(PLUGIN_NAME Streamer)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Definitions REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
    Module.cpp
    Administrator.cpp
    Streamer.cpp
    StreamerImplementation.cpp
    StreamerJsonRpc.cpp
)

if ("${PLUGIN_STREAMER_IMPLEMENTATIONS}" STREQUAL "")
    message(FATAL_ERROR "No players selected! Select at least one player for the Streamer." )
endif ("${PLUGIN_STREAMER_IMPLEMENTATIONS}" STREQUAL "")

# Make it an array
string(REPLACE " " ";" PLUGIN_STREAMER_IMPLEMENTATIONS ${PLUGIN_STREAMER_IMPLEMENTATIONS})

foreach(IMPL ${PLUGIN_STREAMER_IMPLEMENTATIONS})
    add_subdirectory(Implementation/${IMPL})

    list(APPEND PLAYER_DIRS "Implementation/${IMPL}")
    list(APPEND PLAYER_LIBS "PlayerPlatform${IMPL}")
endforeach(IMPL)

include_directories(${PLAYER_DIRS})

target_include_directories(${MODULE_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

# "whole-archive" to force linking of the static player libs
target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        -Wl,--whole-archive ${PLAYER_LIBS} -Wl,--no-whole-archive)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
