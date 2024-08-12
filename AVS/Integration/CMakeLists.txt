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

set(PLUGIN_AVS_PERSISTENT_ROOT_PATH "/root" CACHE STRING "AVS database directory location")
set(PLUGIN_AVS_PERSISTENT_PATH "${PLUGIN_AVS_PERSISTENT_ROOT_PATH}/${NAMESPACE}/AVS")

# platform independent files
install(DIRECTORY
    DESTINATION "${PLUGIN_AVS_DATA_PATH}/${PLUGIN_AVS_NAME}"
    COMPONENT ${NAMESPACE}_Runtime)

# platform dependent files
configure_file( "${CMAKE_CURRENT_LIST_DIR}/${PLUGIN_AVS_PLATFORM}/AlexaClientSDKConfig.json.in"
                "${CMAKE_CURRENT_LIST_DIR}/${PLUGIN_AVS_PLATFORM}/AlexaClientSDKConfig.json"
                @ONLY)
install(FILES "${CMAKE_CURRENT_LIST_DIR}/${PLUGIN_AVS_PLATFORM}/AlexaClientSDKConfig.json"
    DESTINATION "${PLUGIN_AVS_DATA_PATH}/${PLUGIN_AVS_NAME}"
    COMPONENT ${NAMESPACE}_Runtime)

install(FILES "${CMAKE_CURRENT_LIST_DIR}/${PLUGIN_AVS_PLATFORM}/SmartScreenSDKConfig.json"
    DESTINATION "${PLUGIN_AVS_DATA_PATH}/${PLUGIN_AVS_NAME}"
    COMPONENT ${NAMESPACE}_Runtime)

install(FILES "${PLUGIN_AVS_PLATFORM}/asound.conf"
    DESTINATION "/etc/"
    COMPONENT ${NAMESPACE}_Runtime)
