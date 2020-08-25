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

cmake_minimum_required(VERSION 3.3)

option(PLUGIN_JSONRPC "Include JSONRPCExamplePlugin plugin" OFF)
option(PLUGIN_TESTUTILITY "Include TestUtility plugin" OFF)
option(PLUGIN_TESTCONTROLLER "Include TestController plugin" OFF)
option(PLUGIN_FILETRANSFER "Include FileTransfer plugin" OFF)
option(PLUGIN_OUTOFPROCESS "Include OutOfProcess plugin" OFF)
option(PLUGIN_STATECONTROLLER "Include StateController plugin" OFF)

if(PLUGIN_TESTUTILITY)
    add_subdirectory(TestUtility)
endif()

if(PLUGIN_TESTCONTROLLER)
    add_subdirectory(TestController)
endif()

if(PLUGIN_JSONRPC)
    add_subdirectory(JSONRPCPlugin)
    add_subdirectory(JSONRPCClient)
endif()

if(PLUGIN_FILETRANSFER)
    add_subdirectory(FileTransferClient)
endif()

if(PLUGIN_OUTOFPROCESS)
    add_subdirectory(OutOfProcessPlugin)
endif()

if(PLUGIN_STATECONTROLLER)
    add_subdirectory(StateController)
endif()

