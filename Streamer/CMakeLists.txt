set(PLUGIN_NAME Streamer)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(Interfaces REQUIRED)

file(GLOB PLUGIN_INCLUDES *.h)

set(PLUGIN_SOURCES
    Module.cpp
    Frontend.cpp
    Streamer.cpp
    StreamerImplementation.cpp
)

option(STREAMER_IMPLEMENTATION "Define the actual implementation to be used for this player" Stub)
add_subdirectory(Implementation/${STREAMER_IMPLEMENTATION})
include_directories(Implementation/${STREAMER_IMPLEMENTATION})
set(STREAMER_LIBS -Wl,--whole-archive PlayerPlatform -Wl,--no-whole-archive)

# Library definition section
add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES})
target_link_libraries(${MODULE_NAME} ${PLUGINS_LIBRARIES} ${INTERFACES_LIBRARIES} ${ADDITIONAL_LIBRARIES})

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config(${PLUGIN_NAME})
