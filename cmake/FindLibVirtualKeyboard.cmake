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

# - Try to find libvirtualkeyboard
# Once done this will define
#  LIBVIRTUAL_KEYBOARD_FOUND - System has libvirtualkeyboard
#  LIBVIRTUAL_KEYBOARD_INCLUDE_DIRS - The libvirtualkeyboard include directories
#  LIBVIRTUAL_KEYBOARD_LIBRARIES - The libraries needed to use libvirtualkeyboard
#
# Copyright (C) 2016 Metrological.
#
find_package(PkgConfig)
pkg_check_modules(LIBVIRTUAL_KEYBOARD REQUIRED virtualkeyboard)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBVIRTUAL_KEYBOARD DEFAULT_MSG
        LIBVIRTUAL_KEYBOARD_FOUND
        LIBVIRTUAL_KEYBOARD_INCLUDE_DIRS
        LIBVIRTUAL_KEYBOARD_LIBRARIES
        )

mark_as_advanced(LIBVIRTUAL_KEYBOARD_INCLUDE_DIRS LIBVIRTUAL_KEYBOARD_FOUND)
