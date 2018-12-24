cmake_minimum_required(VERSION 2.8)

include(${CMAKE_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/cmake/WPEFramework.cmake)

option(PLUGIN_BLUETOOTH "Include Bluetooth plugin" OFF)
option(PLUGIN_COMMANDER "Include Commander plugin" OFF)
option(PLUGIN_COMPOSITOR "Include Compsitor plugin" OFF)
option(PLUGIN_DEVICEINFO "Include DeviceInfo plugin" OFF)
option(PLUGIN_DSRESOLUTION "Include DSResolution plugin" OFF)
option(PLUGIN_DHCPSERVER "Include DHCPServer plugin" OFF)
option(PLUGIN_DICTIONARY "Include Dictionary plugin" OFF)
option(PLUGIN_IOCONNECTOR "Include IOConnector plugin" OFF)
option(PLUGIN_FRONTPANEL "Include FrontPanel plugin" OFF)
option(PLUGIN_LOCATIONSYNC "Include LocationSync plugin" OFF)
option(PLUGIN_MONITOR "Include Monitor plugin" OFF)
option(PLUGIN_NETWORKCONTROL "Include NetworkControlplugin" OFF)
option(PLUGIN_OPENCDMI "Include OpenCDMi plugin" OFF)
option(PLUGIN_POWER "Include Power plugin" OFF)
option(PLUGIN_REMOTECONTROL "Include RemoteControl plugin" OFF)
option(PLUGIN_STREAMER "Include Streamer plugin" OFF)
option(PLUGIN_SNAPSHOT "Include Snapshot plugin" OFF)
option(PLUGIN_TIMESYNC "Include TimeSync plugin" OFF)
option(PLUGIN_TRACECONTROL "Include TraceControl plugin" OFF)
option(PLUGIN_WEBKITBROWSER "Include WebKitBrowser plugin" OFF)
option(PLUGIN_WEBPROXY "Include WebProxy plugin" OFF)
option(PLUGIN_WEBSERVER "Include WebServer plugin" OFF)
option(PLUGIN_WEBSHELL "Include WebShell plugin" OFF)
option(PLUGIN_TVCONTROL "Include TVControl plugin" OFF)
option(PLUGIN_WIFICONTROL "Include WifiControl plugin" OFF)

# This contains all kinds of plugins (publicely available, so they all need the plugin support library !!
find_package(Plugins REQUIRED)
include_directories(${PLUGINS_INCLUDE_DIR})

# All packages that did not deliver a CMake Find script (and some deprecated scripts that need to be removed) 
# are located in the cmake directory. Include it in the search.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

if(PLUGIN_BLUETOOTH)
    add_subdirectory(Bluetooth)
endif()

if(PLUGIN_COMMANDER)
    add_subdirectory(Commander)
endif()

if(PLUGIN_COMPOSITOR)
    add_subdirectory(Compositor)
endif()

if(PLUGIN_DEVICEINFO)
    add_subdirectory(DeviceInfo)
endif()

if(PLUGIN_DSRESOLUTION)
    add_subdirectory(DSResolution)
endif()

if(PLUGIN_DHCPSERVER)
    add_subdirectory(DHCPServer)
endif()

if(PLUGIN_DICTIONARY)
    add_subdirectory(Dictionary)
endif()

if(PLUGIN_FRONTPANEL)
    add_subdirectory(FrontPanel)
endif()

if(PLUGIN_IOCONNECTOR)
    add_subdirectory(IOConnector)
endif()

if(PLUGIN_SICONTROL)
    add_subdirectory (SIControl)
endif(PLUGIN_SICONTROL)

if(PLUGIN_LOCATIONSYNC)
    add_subdirectory(LocationSync)
endif()

if(PLUGIN_MONITOR)
    add_subdirectory(Monitor)
endif()

if(PLUGIN_NETWORKCONTROL)
    add_subdirectory(NetworkControl)
endif()

if(PLUGIN_OPENCDMI)
    add_subdirectory(OpenCDMi)
endif()

if(PLUGIN_POWER)
    add_subdirectory(Power)
endif()

if(PLUGIN_REMOTECONTROL)
    add_subdirectory(RemoteControl)
endif()

if(PLUGIN_SNAPSHOT)
    add_subdirectory(Snapshot)
endif()

if(PLUGIN_STREAMER)
    add_subdirectory(Streamer)
endif()

if(PLUGIN_SYSTEMDCONNECTOR)
    add_subdirectory(SystemdConnector)
endif()

if(PLUGIN_TIMESYNC)
    add_subdirectory(TimeSync)
endif()

if(PLUGIN_TRACECONTROL)
    add_subdirectory(TraceControl)
endif()

if(PLUGIN_TVCONTROL)
    add_subdirectory(TVControl)
endif(PLUGIN_TVCONTROL)

if(PLUGIN_WEBKITBROWSER)
    add_subdirectory(WebKitBrowser)
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

if(PLUGIN_RTSPCLIENT)
    add_subdirectory(RtspClient)
endif()
