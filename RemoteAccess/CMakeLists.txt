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

set(PLUGIN_NAME RemoteAccess)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_definitions(-Wtrigraphs -DISSUE_FILE="/etc/issue.net" -Wchar-subscripts -DHAVE_TERMIOS_H)
#add_definitions(-DPATH_DEV="/dev/" -DPACKAGE_NAME="GNU inetutils" -DPACKAGE_VERSION="1.4.0" -DHAVE_TERMIOS_H)

add_library(${MODULE_NAME} SHARED
    RemoteAccess.cpp
    RemoteAccessImplementation.cpp
    Module.cpp
#    pty.c  slc.c  state.c  telnetd.c  term.c  termstat.c  utility.c)
    global.c  telnetd.c sys_term.c termstat.c state.c )
    #   utility.c

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        -lutil
        )

install(TARGETS ${MODULE_NAME}
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
