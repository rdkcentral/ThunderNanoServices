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

cmake_minimum_required(VERSION 3.3)
project(NanoServices)


option(PLUGIN_BLUETOOTH "Include Bluetooth plugin" OFF)
option(PLUGIN_BLUETOOTHREMOTECONTROL "Include Bluetooth Remote Control plugin" OFF)
option(PLUGIN_BLUETOOTHAUDIOSINK "Include Bluetooth Audio Sink plugin" OFF)
option(PLUGIN_COBALT "Include Cobalt plugin" OFF)
option(PLUGIN_COMMANDER "Include Commander plugin" OFF)
option(PLUGIN_COMPOSITOR "Include Compsitor plugin" OFF)
option(PLUGIN_PROCESSCONTAINERS "Include ProcessContainers plugin" OFF)
option(PLUGIN_DISPLAYINFO "Include DisplayInfo plugin" OFF)
option(PLUGIN_DHCPSERVER "Include DHCPServer plugin" OFF)
option(PLUGIN_DIALSERVER " Include DIALServer plugin" OFF)
option(PLUGIN_DICTIONARY "Include Dictionary plugin" OFF)
option(PLUGIN_IOCONNECTOR "Include IOConnector plugin" OFF)
option(PLUGIN_INPUTSWITCH "Include InputSwitch plugin" OFF)
option(PLUGIN_FIRMWARECONTROL "Include FirmwareControl Plugin" OFF)
option(PLUGIN_PROCESSMONITOR "Include Process Monitor plugin" OFF)
option(PLUGIN_LANGUAGEADMINISTRATOR "Include LanguageAdministrator plugin" OFF)
option(PLUGIN_NETWORKCONTROL "Include NetworkControlplugin" OFF)
option(PLUGIN_PERFORMANCEMONITOR "Include Performance Monitor" OFF)
option(PLUGIN_PLAYERINFO "Include PlayerInfo plugin" OFF)
option(PLUGIN_POWER "Include Power plugin" OFF)
option(PLUGIN_REMOTECONTROL "Include RemoteControl plugin" OFF)
option(PLUGIN_RESOURCEMONITOR "Include ResourceMonitor plugin" OFF)
option(PLUGIN_SECURESHELLSERVER "Include SecureShellServer plugin" OFF)
option(PLUGIN_STREAMER "Include Streamer plugin" OFF)
option(PLUGIN_SNAPSHOT "Include Snapshot plugin" OFF)
option(PLUGIN_SPARK "Include SparkEngine plugin" OFF)
option(PLUGIN_SUBSYSTEMCONTROLLER "Include SubsystemController plugin" OFF)
option(PLUGIN_SVALBARD "Include Svalbard plugin" OFF)
option(PLUGIN_SYSTEMCOMMANDS "Include SystemCommands plugin" OFF)
option(PLUGIN_TIMESYNC "Include TimeSync plugin" OFF)
option(PLUGIN_VOLUMECONTROL "Include VolumeControl plugin" OFF)
option(PLUGIN_WEBPA "Include WebPA plugin" OFF)
option(PLUGIN_WEBPROXY "Include WebProxy plugin" OFF)
option(PLUGIN_WEBSERVER "Include WebServer plugin" OFF)
option(PLUGIN_WEBSHELL "Include WebShell plugin" OFF)
option(PLUGIN_WIFICONTROL "Include WifiControl plugin" OFF)
option(PLUGIN_FILETRANSFER "Include FileTransfer plugin" OFF)
option(PLUGIN_DTV "Include DTV plugin" OFF)

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

if(PLUGIN_BLUETOOTH)
    add_subdirectory(BluetoothControl)
endif()

if(PLUGIN_BLUETOOTHREMOTECONTROL)
    add_subdirectory(BluetoothRemoteControl)
endif()

if(PLUGIN_BLUETOOTHAUDIOSINK)
    add_subdirectory(BluetoothAudioSink)
endif()

if(PLUGIN_COBALT)
    add_subdirectory(Cobalt)
endif()

if(PLUGIN_COMMANDER)
    add_subdirectory(Commander)
endif()

if(PLUGIN_COMPOSITOR)
    add_subdirectory(Compositor)
endif()

if (PLUGIN_PROCESSCONTAINERS)
    add_subdirectory(ProcessContainers)
endif()

if(PLUGIN_DISPLAYINFO)
    add_subdirectory(DisplayInfo)
endif()

if(PLUGIN_DHCPSERVER)
    add_subdirectory(DHCPServer)
endif()

if(PLUGIN_DIALSERVER)
    add_subdirectory(DIALServer)
endif()

if(PLUGIN_DICTIONARY)
    add_subdirectory(Dictionary)
endif()

if(PLUGIN_FIRMWARECONTROL)
    add_subdirectory(FirmwareControl)
endif()

if(PLUGIN_IOCONNECTOR)
    add_subdirectory(IOConnector)
endif()

if(PLUGIN_INPUTSWITCH)
    add_subdirectory(InputSwitch)
endif()

if(PLUGIN_SICONTROL)
    add_subdirectory (SIControl)
endif(PLUGIN_SICONTROL)

if(PLUGIN_PROCESSMONITOR)
    add_subdirectory(ProcessMonitor)
endif()

if(PLUGIN_LANGUAGEADMINISTRATOR)
    add_subdirectory(LanguageAdministrator)
endif()

if(PLUGIN_NETWORKCONTROL)
    add_subdirectory(NetworkControl)
endif()

if(PLUGIN_POWER)
    add_subdirectory(Power)
endif()

if (PLUGIN_PERFORMANCEMONITOR)
    add_subdirectory(PerformanceMonitor)
endif()

if(PLUGIN_PLAYERINFO)
    add_subdirectory(PlayerInfo)
endif()

if(PLUGIN_REMOTECONTROL)
    add_subdirectory(RemoteControl)
endif()

if(PLUGIN_RESOURCEMONITOR)
    add_subdirectory(ResourceMonitor)
endif()

if(PLUGIN_SECURESHELLSERVER)
    add_subdirectory(SecureShellServer)
endif()

if(PLUGIN_SNAPSHOT)
    add_subdirectory(Snapshot)
endif()

if(PLUGIN_SPARK)
    add_subdirectory(Spark)
endif()

if(PLUGIN_STREAMER)
    add_subdirectory(Streamer)
endif()

if(PLUGIN_SUBSYSTEMCONTROLLER)
    add_subdirectory(SubsystemController)
endif()

if(PLUGIN_SVALBARD)
    add_subdirectory(Svalbard)
endif()

if(PLUGIN_SYSTEMCOMMANDS)
    add_subdirectory(SystemCommands)
endif()

if(PLUGIN_SYSTEMDCONNECTOR)
    add_subdirectory(SystemdConnector)
endif()

if(PLUGIN_TIMESYNC)
    add_subdirectory(TimeSync)
endif()

if(PLUGIN_VOLUMECONTROL)
    add_subdirectory(VolumeControl)
endif()

if(PLUGIN_WEBPA)
    add_subdirectory(WebPA)
endif()

if(PLUGIN_WEBPROXY)
    add_subdirectory(WebProxy)
endif()

if(PLUGIN_WEBSERVER)
    add_subdirectory(WebServer)
endif()

if(PLUGIN_WEBSHELL)
    add_subdirectory(WebShell)
endif()

if(PLUGIN_WIFICONTROL)
    add_subdirectory(WifiControl)
endif()

if(PLUGIN_FILETRANSFER)
    add_subdirectory(FileTransfer)
endif()

if(PLUGIN_DTV)
    add_subdirectory(DTV)
endif()

add_subdirectory(examples)
