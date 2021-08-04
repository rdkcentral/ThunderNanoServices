include_directories( ${WPEFRAMEWORK_GENERICS_INCLUDE_DIR} )
include_directories( ${WPEFRAMEWORK_WEBSOCKET_INCLUDE_DIR} )
include_directories( ${WPEFRAMEWORK_CRYPTALGO_INCLUDE_DIR} )

set (WPEFRAMEWORK_TEST_TESTCONSOLE_LIBS 
        ${CMAKE_THREAD_LIBS_INIT}
        ${CMAKE_DL_LIBS}
        cryptalgo
        websocket 
        generics
)

set (WPEFRAMEWORK_TEST_TESTCONSOLE_SOURCES
        testconsole.cpp
        ../testserver/DataContainer.cpp
)

add_executable(testconsole ${WPEFRAMEWORK_TEST_TESTCONSOLE_SOURCES})
target_link_libraries(testconsole ${WPEFRAMEWORK_TEST_TESTCONSOLE_LIBS})

install(TARGETS testconsole DESTINATION bin)
