cmake_minimum_required(VERSION 3.15)

set(TARGET QASecurityAgentIntegrationTests)

set(QASECURITYAGENT_MODULE_NAME
    ${NAMESPACE}QASecurityAgent)

set(QASECURITYTARGET_MODULE_NAME
    ${NAMESPACE}QASecurityTarget)

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(GTest QUIET)
find_package(thunder_test_support QUIET)

if(NOT GTest_FOUND)
    message(STATUS
        "${TARGET}: GTest not found — skipping QASecurityAgent integration tests")
    return()
endif()

if(NOT thunder_test_support_FOUND)
    message(STATUS
        "${TARGET}: thunder_test_support not found — skipping QASecurityAgent integration tests. "
        "Build Thunder with -DENABLE_TEST_RUNTIME=ON and install it first.")
    return()
endif()

add_executable(${TARGET}
    SecurityAgentTestSuite.cpp
    ../QASecurityAgent/QASecurityContext.cpp
    Module.cpp
)

target_include_directories(${TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../QASecurityAgent
        ${CMAKE_CURRENT_SOURCE_DIR}/../QASecurityTarget
)

add_dependencies(${TARGET}
    ${QASECURITYAGENT_MODULE_NAME}
    ${QASECURITYTARGET_MODULE_NAME}
)

target_compile_definitions(${TARGET}
    PRIVATE
        MODULE_NAME=QASecurityAgentTestSuite

        QASECURITYAGENT_TEST_CALLSIGN="QASecurityAgent"
        QASECURITYAGENT_TEST_CLASSNAME="QASecurityAgent"
        QASECURITYAGENT_TEST_LOCATOR="$<TARGET_FILE_NAME:${QASECURITYAGENT_MODULE_NAME}>"
        QASECURITYAGENT_TEST_PLUGIN_PATH="$<TARGET_FILE_DIR:${QASECURITYAGENT_MODULE_NAME}>"

        QASECURITYTARGET_TEST_CALLSIGN="QASecurityTarget"
        QASECURITYTARGET_TEST_CLASSNAME="QASecurityTarget"
        QASECURITYTARGET_TEST_LOCATOR="$<TARGET_FILE_NAME:${QASECURITYTARGET_MODULE_NAME}>"
)


if(BUILD_REFERENCE)
    target_compile_definitions(
        ${TARGET}
        PRIVATE
            BUILD_REFERENCE=${BUILD_REFERENCE}
    )
endif()

target_link_libraries(${TARGET}
    PRIVATE
        ${QASECURITYAGENT_MODULE_NAME}
        thunder_test_support::thunder_test_support
        GTest::gtest
        GTest::gtest_main
)

set_target_properties(${TARGET} PROPERTIES
    CXX_STANDARD ${CXX_STD}
    CXX_STANDARD_REQUIRED YES
    RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/tests/Security"
)

add_custom_command(
    TARGET ${TARGET}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${QASECURITYTARGET_MODULE_NAME}>"
        "$<TARGET_FILE_DIR:${QASECURITYAGENT_MODULE_NAME}>"
)

add_test( NAME ${TARGET} COMMAND ${TARGET})

set_tests_properties(${TARGET}
    PROPERTIES
        ENVIRONMENT
            "LD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}:$<TARGET_FILE_DIR:${QASECURITYAGENT_MODULE_NAME}>:$ENV{LD_LIBRARY_PATH}"
)