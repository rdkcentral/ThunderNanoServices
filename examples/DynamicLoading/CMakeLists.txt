# If not stated otherwise in this file or this component's license file the
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

option(EXAMPLE_DYNAMICLOADING_YIN "Include dynamic loading Yin example plugin" OFF)
option(EXAMPLE_DYNAMICLOADING_YANG "Include dynamic loading Yang example plugin" OFF)
set(EXAMPLE_DYNAMICLOADING_TARGET "/tmp/dynamic" CACHE STRING "Path to store the plugin libraries and artifacts")

if(EXAMPLE_DYNAMICLOADING_YIN)
    add_subdirectory(Yin)
endif()

if(EXAMPLE_DYNAMICLOADING_YANG)
    add_subdirectory(Yang)
endif()
