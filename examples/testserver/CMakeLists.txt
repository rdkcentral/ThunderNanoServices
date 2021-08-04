include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set (WPEFRAMEWORK_TEST_TESTSERVER_LIBS 
        ${CMAKE_THREAD_LIBS_INIT}
        ${CMAKE_DL_LIBS}
        cryptalgo
        websocket 
        generics
        devices
)

set (WPEFRAMEWORK_TEST_TESTSERVER_SOURCES
        testserver.cpp
        DataContainer.cpp
)

add_executable(testserver ${WPEFRAMEWORK_TEST_TESTSERVER_SOURCES})
target_link_libraries(testserver ${WPEFRAMEWORK_TEST_TESTSERVER_LIBS})

install(TARGETS testserver DESTINATION bin)
