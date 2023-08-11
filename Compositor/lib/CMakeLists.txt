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

if("${PLUGIN_COMPOSITOR_IMPLEMENTATION}" STREQUAL "Nexus")
    if (NOT PLUGIN_COMPOSITOR_NXSERVER)
        set(NEXUS_SERVER_EXTERNAL ON CACHE STRING "Enable external nxserver")
    endif()

    if (COMPOSITOR_IMPLEMENTATION_PATH)
        add_subdirectory(${COMPOSITOR_IMPLEMENTATION_PATH})
    else()
        include(GetExternalCode)
        set(COMPOSITOR_IMPLEMENTATION_VERSION "master" CACHE STRING "Compositor implementation version: commit_id_or_tag_or_branch_name")
        set(COMPOSITOR_IMPLEMENTATION_REPOSITORY "https://code.rdkcentral.com/r/soc/broadcom/components/rdkcentral/thundernanoservices/Compositor" CACHE STRING "Compositor implementation repository")
        GetExternalCode(
            GIT_REPOSITORY ${COMPOSITOR_IMPLEMENTATION_REPOSITORY}
            GIT_VERSION ${COMPOSITOR_IMPLEMENTATION_VERSION}
            SOURCE_DIR "Nexus"
        )
        add_subdirectory(Nexus)
    endif()
else()
    add_subdirectory (${PLUGIN_COMPOSITOR_IMPLEMENTATION})
endif()
