set(PLAYER_NAME QAM)
message("Building ${PLAYER_NAME} Streamer....")

find_package(${NAMESPACE}Broadcast REQUIRED)
find_package(NXCLIENT REQUIRED)

set(LIB_NAME PlayerPlatform${PLAYER_NAME})

add_library(${LIB_NAME} STATIC
     PlayerImplementation.cpp)

set_target_properties(${LIB_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

if (PLUGIN_STREAMER_TS_SCANNING)
    target_compile_definitions(${LIB_NAME}
        PRIVATE
            ENABLE_TS_SCANNING)
endif(PLUGIN_STREAMER_TS_SCANNING)

target_include_directories(${LIB_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../)

target_link_libraries(${LIB_NAME}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Broadcast::${NAMESPACE}Broadcast)

install(TARGETS ${LIB_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
