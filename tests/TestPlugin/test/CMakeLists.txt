find_package(thunder_test_support QUIET)
find_package(GTest QUIET)

if(NOT thunder_test_support_FOUND)
    message(STATUS "thunder_test_support not found — skipping TestPlugin integration test")
    return()
endif()

if(NOT GTest_FOUND)
    message(STATUS "GTest not found — skipping TestPlugin integration test")
    return()
endif()

set(TEST_PLUGIN_PATH "" CACHE PATH "Directory containing libThunderTestPlugin.so")

if(NOT TEST_PLUGIN_PATH)
    message(STATUS "TEST_PLUGIN_PATH not set — skipping TestPlugin integration test")
    return()
endif()

set(TARGET thunder_testplugin_test)

add_executable(${TARGET}
    TestPluginTest.cpp
    Module.cpp
)

target_compile_definitions(${TARGET}
    PRIVATE
        MODULE_NAME=TestPluginTest
        TEST_PLUGIN_PATH="${TEST_PLUGIN_PATH}"
)

if(BUILD_REFERENCE)
    target_compile_definitions(${TARGET} PRIVATE BUILD_REFERENCE=${BUILD_REFERENCE})
endif()

target_link_libraries(${TARGET}
    PRIVATE
        thunder_test_support::thunder_test_support
        GTest::GTest
        GTest::Main
)

install(TARGETS ${TARGET} DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Test)
add_test(NAME ${TARGET} COMMAND ${TARGET})