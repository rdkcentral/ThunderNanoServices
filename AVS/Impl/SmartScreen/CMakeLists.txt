# If not stated otherwise in this file or this component's license file the
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

set(MODULE_NAME SmartScreen)

find_package(Thunder REQUIRED)
find_package(AlexaClientSDK REQUIRED)
find_package(AlexaSmartScreenSDK REQUIRED)

find_package(GStreamer REQUIRED)
find_package(PortAudio)
find_package(Asio REQUIRED)
find_package(PryonLite)

set(PLUGIN_AVS_SMARTSCREEN_SOURCES)
list(APPEND PLUGIN_AVS_SMARTSCREEN_SOURCES
    SmartScreen.cpp
    ../Module.cpp
    ../ThunderLogger.cpp
)

add_definitions(-DENABLE_SMART_SCREEN_SUPPORT)

if(PLUGIN_AVS_ENABLE_KWD_SUPPORT)
    list(APPEND PLUGIN_AVS_SMARTSCREEN_SOURCES ../PryonKeywordDetector.cpp)
    add_definitions(-DKWD_PRYON)
endif()

set_source_files_properties(SmartScreen.cpp PROPERTIES COMPILE_FLAGS "-fexceptions" )
add_library(${MODULE_NAME} ${PLUGIN_AVS_SMARTSCREEN_SOURCES})

set_target_properties(${MODULE_NAME}
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON)

target_include_directories(${MODULE_NAME}
    PUBLIC
        ../
    )

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        AlexaClientSDK::AlexaClientSDK
        AlexaSmartScreenSDK::AlexaSmartScreenSDK)

if(PLUGIN_AVS_ENABLE_KWD_SUPPORT)
    if(PryonLite_FOUND)
        target_include_directories(${MODULE_NAME} PUBLIC ${PRYON_LITE_INCLUDES})
        target_link_libraries(${MODULE_NAME} PRIVATE PryonLite::PryonLite)
    else()
        message(FATAL_ERROR "Missing pryon_lite library!")
    endif()
endif()

if(GSTREAMER_FOUND)
    target_include_directories(${MODULE_NAME} PUBLIC ${GSTREAMER_INCLUDES})
    target_link_libraries(${MODULE_NAME}
        PRIVATE
            ${GSTREAMER_LIBRARIES}
            MediaPlayer)
    add_definitions(-DGSTREAMER_MEDIA_PLAYER)
endif()

if(PORTAUDIO_FOUND)
    target_include_directories(${MODULE_NAME} PUBLIC PortAudio::PortAudio)
    target_link_libraries(${MODULE_NAME} PRIVATE ${PORTAUDIO_LIBRARIES})
    add_definitions(-DPORTAUDIO)
endif()

if(ASIO_FOUND)
    target_include_directories(${MODULE_NAME} PRIVATE Asio::Asio)
    add_definitions(-DASIO_STANDALONE)
else()
    message(FATAL_ERROR "MISSING Asio (Standalone) Library")
endif()

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Runtime)
