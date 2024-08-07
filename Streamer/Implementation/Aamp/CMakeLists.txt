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

set(PLAYER_NAME Aamp)
message("Building ${PLAYER_NAME} Streamer....")

find_package(Aamp REQUIRED)
find_package(GLIB REQUIRED)
find_package(GStreamer REQUIRED)
find_package(GStreamerVideo REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)

set(LIB_NAME PlayerPlatform${PLAYER_NAME})

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(${LIB_NAME} STATIC
    PlayerImplementation.cpp)

target_include_directories(${LIB_NAME}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/../../
        ${GLIB_INCLUDE_DIRS})

target_link_libraries(${LIB_NAME}
    PRIVATE
        Aamp::Aamp
        GStreamer::GStreamer
        GStreamerVideo::GStreamerVideo
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${GLIB_GIO_LIBRARIES}
        ${GLIB_LIBRARIES})

install(TARGETS ${LIB_NAME}
	DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Development)
