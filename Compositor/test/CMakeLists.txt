# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2023 Metrological
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

# TODO: fix warnings in these tests
add_compile_options( 
    -Wno-unused-variable 
    -Wno-unused-function 
    -Wno-unused-parameter)
    
option(BUILD_COMPOSITOR_TEST_CLIENT "Build compositor test client" OFF)
option(BUILD_COMPOSITOR_TEST_SERVER "Build compositor test server" OFF)

if (BUILD_COMPOSITOR_TEST_CLIENT)
    add_subdirectory(compositortest)
endif()
if (BUILD_COMPOSITOR_TEST_SERVER)
add_subdirectory(compositorserver)
endif()


