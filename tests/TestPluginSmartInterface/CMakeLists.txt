set(TEST_PLUGINSMARTINTERFACE_PROVIDER_TARGET ${NAMESPACE}TestSmartProvider)
set(TEST_PLUGINSMARTINTERFACE_CONSUMER_TARGET ${NAMESPACE}TestSmartConsumer)

add_subdirectory(Provider)
add_subdirectory(Consumer)

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(GTest QUIET)
find_package(thunder_test_support REQUIRED)

if(NOT GTest_FOUND)
    message(STATUS "${TARGET}: GTest not found — skipping integration tests")
    return()
endif()

if(NOT thunder_test_support_FOUND)
    message(STATUS "${TARGET}: thunder_test_support not found — skipping integration tests. "
                   "Build Thunder with -DENABLE_TEST_RUNTIME=ON and install it first.")
    return()
endif()

option(TESTPLUGINSMARTINTERFACE_TESTS "Build integration tests" OFF)

if(TESTPLUGINSMARTINTERFACE_TESTS)
    add_subdirectory(tests)
endif()
