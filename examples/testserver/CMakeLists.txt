find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Cryptalgo REQUIRED)
find_package(${NAMESPACE}WebSocket REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

set (WPEFRAMEWORK_TEST_TESTSERVER_SOURCES
        testserver.cpp
        DataContainer.cpp
)

add_executable(testserver ${WPEFRAMEWORK_TEST_TESTSERVER_SOURCES})

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(testserver
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Cryptalgo::${NAMESPACE}Cryptalgo
        ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

install(TARGETS testserver DESTINATION ${CMAKE_INSTALL_BINDIR})
