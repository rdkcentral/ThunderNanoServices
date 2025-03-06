# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2022 Metrological
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

project(Tests)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)
message("Setup ${PROJECT_NAME} v${PROJECT_VERSION}")

option(STORE_TEST "Utility to verify PersistentStore and Dictionary Plugin behaviour." OFF)
option(TEST_AUTOMATION_TOOLS "Utility to verify Thunder functions trigerred by Automation Test Tools." OFF)
option(PLUGIN_TESTCONTROLLER "Include TestController plugin" OFF)
option(PLUGIN_TESTUTILITY "Include TestUtility plugin" OFF)

if(STORE_TEST)
    add_subdirectory(StoreTest)
endif()

if(TEST_AUTOMATION_TOOLS)
    add_subdirectory(TestAutomationComRpc)
    add_subdirectory(TestAutomationMemory)
    add_subdirectory(TestAutomationUtils)
endif()

if(PLUGIN_TESTCONTROLLER)
    add_subdirectory(TestController)
endif()

if(PLUGIN_TESTUTILITY)
    add_subdirectory(TestUtility)
endif()

