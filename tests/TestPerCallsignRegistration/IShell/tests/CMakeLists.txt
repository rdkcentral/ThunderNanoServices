set(TARGET PerCallsignRegistrationIShellTests)

find_package(GTest QUIET)
find_package(thunder_test_support QUIET)

if(NOT GTest_FOUND)
	message(STATUS "${TARGET}: GTest not found - skipping IShell per-callsign registration tests")
	return()
endif()

if(NOT thunder_test_support_FOUND)
	message(STATUS "${TARGET}: thunder_test_support not found - skipping IShell per-callsign registration tests. Build Thunder with -DENABLE_TEST_RUNTIME=ON and install it first.")
	return()
endif()

add_executable(${TARGET}
	PerCallsignRegistrationIShellTest.cpp
	Module.cpp
)

add_dependencies(${TARGET} ${MODULE_NAME})

target_compile_definitions(${TARGET}
	PRIVATE
		MODULE_NAME=PerCallsignRegistrationIShellTests
		PERCALLSIGNREGISTRATION_ISHELL_TEST_CLASSNAME="TestPerCallsignRegistrationIShell"
		PERCALLSIGNREGISTRATION_ISHELL_TEST_LOCATOR="$<TARGET_FILE_NAME:${MODULE_NAME}>"
		PERCALLSIGNREGISTRATION_ISHELL_TEST_PLUGIN_PATH="$<TARGET_FILE_DIR:${MODULE_NAME}>"
)

if(BUILD_REFERENCE)
	target_compile_definitions(${TARGET} PRIVATE BUILD_REFERENCE=${BUILD_REFERENCE})
endif()

target_link_libraries(${TARGET}
	PRIVATE
		thunder_test_support::thunder_test_support
		GTest::gtest
		GTest::gtest_main
)

set_target_properties(${TARGET} PROPERTIES
	CXX_STANDARD ${CXX_STD}
	CXX_STANDARD_REQUIRED YES
)

add_test(NAME ${TARGET} COMMAND ${TARGET})
set_tests_properties(${TARGET} PROPERTIES
	ENVIRONMENT
		"LD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}:$<TARGET_FILE_DIR:${MODULE_NAME}>:$ENV{LD_LIBRARY_PATH}"
)
