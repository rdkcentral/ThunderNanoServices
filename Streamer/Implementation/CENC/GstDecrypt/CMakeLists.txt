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

set(GST_CENC_PLUGIN_NAME gstdashdecrypt)

find_package(PkgConfig REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)

pkg_check_modules(GST 
    REQUIRED 
        gstreamer-1.0>=1.4
        gstreamer-sdp-1.0>=1.4
        gstreamer-video-1.0>=1.4
        gstreamer-app-1.0>=1.4
        gstreamer-base-1.0>=1.4)

add_library(${GST_CENC_PLUGIN_NAME} SHARED "")

target_include_directories(${GST_CENC_PLUGIN_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${GST_INCLUDE_DIRS})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ocdm)

target_sources(${GST_CENC_PLUGIN_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/GstCencDecrypt.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EncryptedBuffer.cpp)

target_link_libraries(${GST_CENC_PLUGIN_NAME}
    PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins 
        ${GST_LIBRARIES}
        gstdashdecrypt_ocdm)

install(TARGETS ${GST_CENC_PLUGIN_NAME}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/gstreamer-1.0)
     
