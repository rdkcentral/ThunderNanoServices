# Build platformserver
add_executable(platformserver platformserver.cpp)

target_link_libraries(platformserver 
    PRIVATE 
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

install(TARGETS platformserver 
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
