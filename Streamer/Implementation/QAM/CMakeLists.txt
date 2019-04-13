message("Building QAM Streamer....")

find_package(${NAMESPACE}Broadcast REQUIRED)
find_package(NXCLIENT REQUIRED)

add_library(PlayerPlatform STATIC
     PlayerImplementation.cpp)

set_target_properties(PlayerPlatform PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

if (PLUGIN_STREAMER_TS_SCANNING)
    target_compile_definitions( PlayerPlatform
        PRIVATE
            ENABLE_TS_SCANNING)
endif(PLUGIN_STREAMER_TS_SCANNING)

target_link_libraries(PlayerPlatform
    PRIVATE
        ${NAMESPACE}Broadcast::${NAMESPACE}Broadcast)

install(TARGETS PlayerPlatform
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
