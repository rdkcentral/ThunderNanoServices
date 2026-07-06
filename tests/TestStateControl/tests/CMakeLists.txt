# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2020 Metrological
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

# ==========================================================================
# StateControlIntegrationTests
#
# In-process GTest suite that exercises IStateControl via ThunderTestRuntime.
# The TestStateControl plugin shared library must be present on the library
# search path (or installed) for the tests to activate the plugin at runtime.
# ==========================================================================

set(TARGET StateControlIntegrationTests)

find_package(GTest QUIET)
find_package(thunder_test_support QUIET)

if(NOT GTest_FOUND)
    message(STATUS "${TARGET}: GTest not found — skipping TestStateControl integration tests")
    return()
endif()

if(NOT thunder_test_support_FOUND)
    message(STATUS "${TARGET}: thunder_test_support not found — skipping TestStateControl integration tests. "
                   "Build Thunder with -DENABLE_TEST_RUNTIME=ON and install it first.")
    return()
endif()

add_executable(${TARGET}
    StateControlTestSuite.cpp
    Module.cpp
)

add_dependencies(${TARGET} ${MODULE_NAME})

target_compile_definitions(${TARGET}
    PRIVATE
        MODULE_NAME=StateControlTestSuite
        TESTSTATECONTROL_TEST_CALLSIGN="TestStateControl"
        TESTSTATECONTROL_TEST_CLASSNAME="TestStateControl"
        TESTSTATECONTROL_TEST_LOCATOR="$<TARGET_FILE_NAME:${MODULE_NAME}>"
        TESTSTATECONTROL_TEST_PLUGIN_PATH="$<TARGET_FILE_DIR:${MODULE_NAME}>"
)

if(BUILD_REFERENCE)
    target_compile_definitions(${TARGET} PRIVATE BUILD_REFERENCE=${BUILD_REFERENCE})
endif()

target_link_libraries(${TARGET}
    PRIVATE
        thunder_test_support::thunder_test_support
        GTest::gtest
        GTest::gtest_main
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
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
