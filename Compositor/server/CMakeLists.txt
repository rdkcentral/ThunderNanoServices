# Build platformserver
add_executable(platformserver platformserver.cpp)
target_link_libraries(platformserver ${PLUGINS_LIBRARIES})

install(TARGETS platformserver DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
