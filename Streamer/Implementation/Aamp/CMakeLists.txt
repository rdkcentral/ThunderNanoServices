set(PLAYER_NAME Aamp)
message("Building ${PLAYER_NAME} Streamer....")

find_package(Aamp REQUIRED)
find_package(GLIB REQUIRED)
find_package(GStreamer REQUIRED)
find_package(GStreamerVideo REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)

set(LIB_NAME PlayerPlatform${PLAYER_NAME})

add_library(${LIB_NAME} STATIC
    PlayerImplementation.cpp)

target_include_directories(${LIB_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../
        ${GLIB_INCLUDE_DIRS})

target_link_libraries(${LIB_NAME}
    PRIVATE
        AAMP::AAMP
        GStreamer::GStreamer
        GStreamerVideo::GStreamerVideo
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${GLIB_GIO_LIBRARIES}
        ${GLIB_LIBRARIES})

install(TARGETS ${LIB_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
