find_package(Aamp REQUIRED)

if (AAMP_FOUND)
    find_package(GLIB REQUIRED)
    find_package(GStreamer REQUIRED)

    add_library(PlayerPlatform STATIC
        PlayerImplementation.cpp)

    target_link_libraries(PlayerPlatform
        PRIVATE 
            ${GLIB_GIO_LIBRARIES} 
            ${GLIB_LIBRARIES} 
            ${GSTREAMER_LIBRARIES})

    target_include_directories(PlayerPlatform 
        PRIVATE  
            ${GLIB_INCLUDE_DIRS} 
            ${GSTREAMER_INCLUDE_DIRS}})

    install(TARGETS PlayerPlatform 
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
endif()
