message("Building AAMP Streamer....")
find_package(Aamp REQUIRED)

find_package(GLIB REQUIRED)
find_package(GStreamer REQUIRED)
find_package(GStreamerVideo REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)

add_library(PlayerPlatform STATIC
    PlayerImplementation.cpp)

target_link_libraries(PlayerPlatform
    PRIVATE
        AAMP::AAMP
        GStreamer::GStreamer
        GStreamerVideo::GStreamerVideo
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${GLIB_GIO_LIBRARIES}
        ${GLIB_LIBRARIES})

target_include_directories(PlayerPlatform
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../
        ${GLIB_INCLUDE_DIRS})

install(TARGETS PlayerPlatform
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
