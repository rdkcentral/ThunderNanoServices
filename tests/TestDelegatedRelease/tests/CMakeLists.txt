# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2026 Metrological
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(TARGET TestDelegatedReleaseIntegrationTests)

find_package(GTest QUIET)
find_package(thunder_test_support QUIET)

if(NOT GTest_FOUND)
    message(STATUS "${TARGET}: GTest not found - skipping TestDelegatedRelease integration tests")
    return()
endif()

if(NOT thunder_test_support_FOUND)
    message(STATUS "${TARGET}: thunder_test_support not found - skipping TestDelegatedRelease integration tests")
    return()
endif()

add_executable(${TARGET}
    DelegatedReleaseTestSuite.cpp
    Module.cpp
)

add_dependencies(${TARGET} ${MODULE_NAME})

target_compile_definitions(${TARGET}
    PRIVATE
        MODULE_NAME=TestDelegatedReleaseIntegrationTests
        TESTDELEGATEDRELEASE_TEST_CALLSIGN="TestDelegatedRelease"
        TESTDELEGATEDRELEASE_TEST_CLASSNAME="TestDelegatedRelease"
        TESTDELEGATEDRELEASE_TEST_LOCATOR="$<TARGET_FILE_NAME:${MODULE_NAME}>"
        TESTDELEGATEDRELEASE_TEST_PLUGIN_PATH="$<TARGET_FILE_DIR:${MODULE_NAME}>"
        TESTDELEGATEDRELEASE_TEST_PROXYSTUB_PATH="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/proxystubs"
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

set_target_properties(${TARGET} PROPERTIES
    CXX_STANDARD ${CXX_STD}
    CXX_STANDARD_REQUIRED YES
)

add_test(NAME ${TARGET} COMMAND ${TARGET})
set_tests_properties(${TARGET} PROPERTIES
    ENVIRONMENT
        "PATH=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}:$ENV{PATH};LD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}:${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/proxystubs:$<TARGET_FILE_DIR:${MODULE_NAME}>:$ENV{LD_LIBRARY_PATH}"
)