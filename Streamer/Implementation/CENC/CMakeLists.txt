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

set(PLAYER_NAME CENC)

find_library(GSTCENCDECRYPTOR gstcencdecrypt)
find_package(${NAMESPACE}Core REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST 
    REQUIRED 
        gstreamer-1.0>=1.4
        gstreamer-sdp-1.0>=1.4
        gstreamer-video-1.0>=1.4
        gstreamer-app-1.0>=1.4
        gstreamer-base-1.0>=1.4)

set(LIB_NAME PlayerPlatform${PLAYER_NAME})

add_library(${LIB_NAME} STATIC
     PlayerImplementation.cpp)

set_target_properties(${LIB_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_include_directories(${LIB_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../
        ${GST_INCLUDE_DIRS})

target_link_libraries(${LIB_NAME}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${GST_LIBRARIES}
        ${GSTCENCDECRYPTOR})

install(TARGETS ${LIB_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
