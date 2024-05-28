find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Cryptalgo REQUIRED)
find_package(${NAMESPACE}WebSocket REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

set (PLUGIN_TEST_TESTCONSOLE_SOURCES
        testconsole.cpp
        ../testserver/DataContainer.cpp
)

add_executable(testconsole ${PLUGIN_TEST_TESTCONSOLE_SOURCES})

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(testconsole
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Cryptalgo::${NAMESPACE}Cryptalgo
        ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

install(TARGETS testconsole DESTINATION ${CMAKE_INSTALL_BINDIR})
