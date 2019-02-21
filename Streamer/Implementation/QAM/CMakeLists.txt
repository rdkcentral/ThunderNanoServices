message("Building QAM Streamer....")

find_package(${NAMESPACE}Broadcast REQUIRED)
find_package(NXCLIENT REQUIRED)

add_library(PlayerPlatform STATIC
     PlayerImplementation.cpp)

if (PLUGIN_STREAMER_TS_SCANNING)
    target_compile_definitions( PlayerPlatform 
        PRIVATE 
            ENABLE_TS_SCANNING)
endif(PLUGIN_STREAMER_TS_SCANNING)

target_link_libraries(PlayerPlatform 
    PRIVATE 
        ${NAMESPACE}Broadcast::${NAMESPACE}Broadcast
        NXCLIENT::NXCLIENT)

install(TARGETS PlayerPlatform 
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
