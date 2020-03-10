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

# - Try to find Systemd
# Once done this will define

#  SYSTEMD_FOUND - System has libsystemd
#  SYSTEMD_INCLUDE_DIRS - The libsystemd include directories
#  SYSTEMD_LIBRARIES - The libraries needed to use libsystemd
#  SYSTEMD_DEFINITIONS - The libraries needed to use libsystemd

find_package(PkgConfig)
pkg_check_modules(PC_SYSTEMD QUIET libsystemd)

find_library(SYSTEMD_LIBRARIES NAMES systemd ${PC_SYSTEMD_LIBRARY_DIRS})
find_path(SYSTEMD_INCLUDE_DIRS systemd/sd-login.h HINTS ${PC_SYSTEMD_INCLUDE_DIRS})
set(SYSTEMD_DEFINITIONS ${PC_SYSTEMD_CFLAGS_OTHER})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SYSTEMD DEFAULT_MSG SYSTEMD_INCLUDE_DIRS SYSTEMD_LIBRARIES)
mark_as_advanced(SYSTEMD_FOUND SYSTEMD_INCLUDE_DIRS SYSTEMD_LIBRARIES SYSTEMD_DEFINITIONS)
