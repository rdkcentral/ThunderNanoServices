# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the License);
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

find_path(NXSERVER_INCLUDE_DIR nexus_platform_client.h
        PATHS usr/include/
        PATH_SUFFIXES refsw
        )

find_library(PM_LIBRARY NAMES libpmlib.a)
set (BCM_PM_LIBRARIES ${PM_LIBRARY} CACHE PATH "path to pmlib library")
set (BCM_PM_INCLUDE_DIRS ${NXSERVER_INCLUDE_DIR} CACHE PATH "Path to header")
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BCM_PM DEFAULT_MSG BCM_PM_INCLUDE_DIRS BCM_PM_LIBRARIES)

mark_as_advanced(BCM_PM_INCLUDE_DIRS BCM_PM_LIBRARIES)
