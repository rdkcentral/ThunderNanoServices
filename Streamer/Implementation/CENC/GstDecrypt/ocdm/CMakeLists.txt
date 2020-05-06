# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
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
 

find_package(ocdm REQUIRED)
set(GST_CENC_PLUGIN_IMPLEMENTATION gstdashdecrypt_ocdm)

add_library(${GST_CENC_PLUGIN_IMPLEMENTATION} STATIC "")

target_include_directories(${GST_CENC_PLUGIN_IMPLEMENTATION}
    PUBLIC
        ${GST_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_sources(${GST_CENC_PLUGIN_IMPLEMENTATION} 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Decryptor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Exchanger.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ResponseCallback.cpp)

target_link_libraries(${GST_CENC_PLUGIN_IMPLEMENTATION}
        PUBLIC
            ocdm::ocdm
            ${OCDM_LIBRARIES})
