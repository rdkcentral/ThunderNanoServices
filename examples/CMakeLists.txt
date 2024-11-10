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

project(Examples)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)

project_version(1.0.0)
message("Setup ${PROJECT_NAME} v${PROJECT_VERSION}")

option(EXAMPLE_COMRPCCLIENT "Include COMRPCClient example" OFF)
option(PLUGIN_JSONRPC "Include JSONRPCExamplePlugin plugin" OFF)
option(PLUGIN_FILETRANSFER "Include FileTransfer plugin" OFF)
option(PLUGIN_OUTOFPROCESS "Include OutOfProcess plugin" OFF)
option(PLUGIN_STATECONTROLLER "Include StateController plugin" OFF)
option(PLUGIN_CONFIGUPDATEEXAMPLE "Include Sample plugin to get/set plugins config" OFF)
option(EXAMPLE_CLIENT_SERVER "Include Client Server Echo test suite" OFF)
option(EXAMPLE_IOCONNECTOR_TEST "Include IOConnector test suite" OFF)
option(EXAMPLE_MESSAGECONTROL_UDP_CLIENT "Include UDP output client for message control" OFF)
option(EXAMPLE_DYNAMICLOADING "Include dynamic loading examples" OFF)
option(EXAMPLE_SIMPLECOMRPC_TEST "Include Simple COMRPC test client" OFF)
option(EXAMPLE_PLUGINSMARTINTERFACETYPE_EXAMPLE "Include PluginSmartInterfaceType examples" OFF)
option(EXAMPLE_DYNAMICJSONRPCERRORMESSAGE_EXAMPLE "Include DynamicJSONRPCErrorMessage example" OFF)

if(EXAMPLE_COMRPCCLIENT)
    add_subdirectory(COMRPCClient)
endif()

if(PLUGIN_JSONRPC)
    add_subdirectory(JSONRPC/Plugin)
    add_subdirectory(JSONRPC/Client)
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

if(PLUGIN_CONFIGUPDATEEXAMPLE)
    add_subdirectory(ConfigUpdateExample)
endif()

if(EXAMPLE_CLIENT_SERVER)
    add_subdirectory(testserver)
    add_subdirectory(testconsole)
endif()

if(EXAMPLE_IOCONNECTOR_TEST)
    add_subdirectory(IOConnectorTest)
endif()

if(EXAMPLE_MESSAGECONTROL_UDP_CLIENT)
    add_subdirectory(MessageControlUDPClient)
endif()

if (EXAMPLE_DYNAMICLOADING)
    add_subdirectory(DynamicLoading)
endif()

if (EXAMPLE_PLUGINSMARTINTERFACETYPE_EXAMPLE)
    add_subdirectory(PluginSmartInterfaceType)
endif()

if (EXAMPLE_SIMPLECOMRPC_TEST)
    add_subdirectory(SimpleCOMRPC/PluginServer)
    add_subdirectory(SimpleCOMRPC/Client)
endif()

if (EXAMPLE_DYNAMICJSONRPCERRORMESSAGE_EXAMPLE)
    add_subdirectory(DynamicJSONRPCErrorMessage)
endif()

