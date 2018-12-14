cmake_minimum_required(VERSION 2.8)

include($ENV{STAGING_DIR}${CMAKE_INSTALL_PREFIX}/include/cmake/tools.cmake)

option(WPEFRAMEWORK_PLUGIN_BLUETOOTH "Include Bluetooth plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_COMMANDER "Include Commander plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_COMPOSITOR "Include Compsitor plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_DEVICEINFO "Include DeviceInfo plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_DSRESOLUTION "Include DSResolution plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_DHCPSERVER "Include DHCPServer plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_DICTIONARY "Include Dictionary plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_IOCONNECTOR "Include IOConnector plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_FRONTPANEL "Include FrontPanel plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_LOCATIONSYNC "Include LocationSync plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_MONITOR "Include Monitor plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_NETWORKCONTROL "Include NetworkControlplugin" OFF)
option(WPEFRAMEWORK_PLUGIN_OPENCDMI "Include OpenCDMi plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_POWER "Include Power plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_REMOTECONTROL "Include RemoteControl plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_STREAMER "Include Streamer plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_SNAPSHOT "Include Snapshot plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_TIMESYNC "Include TimeSync plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_TRACECONTROL "Include TraceControl plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_WEBKITBROWSER "Include WebKitBrowser plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_WEBPROXY "Include WebProxy plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_WEBSERVER "Include WebServer plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_WEBSHELL "Include WebShell plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_TVCONTROL "Include TVControl plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_WIFICONTROL "Include WifiControl plugin" OFF)

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

if(WPEFRAMEWORK_PLUGIN_BLUETOOTH)
    add_subdirectory(Bluetooth)
endif()

if(WPEFRAMEWORK_PLUGIN_COMMANDER)
    add_subdirectory(Commander)
endif()

if(WPEFRAMEWORK_PLUGIN_COMPOSITOR)
    add_subdirectory(Compositor)
endif()

if(WPEFRAMEWORK_PLUGIN_DEVICEINFO)
    add_subdirectory(DeviceInfo)
endif()

if(WPEFRAMEWORK_PLUGIN_DSRESOLUTION)
    add_subdirectory(DSResolution)
endif()

if(WPEFRAMEWORK_PLUGIN_DHCPSERVER)
    add_subdirectory(DHCPServer)
endif()

if(WPEFRAMEWORK_PLUGIN_DICTIONARY)
    add_subdirectory(Dictionary)
endif()

if(WPEFRAMEWORK_PLUGIN_FRONTPANEL)
    add_subdirectory(FrontPanel)
endif()

if(WPEFRAMEWORK_PLUGIN_IOCONNECTOR)
    add_subdirectory(IOConnector)
endif()

if(WPEFRAMEWORK_PLUGIN_SICONTROL)
    add_subdirectory (SIControl)
endif(WPEFRAMEWORK_PLUGIN_SICONTROL)

if(WPEFRAMEWORK_PLUGIN_LOCATIONSYNC)
    add_subdirectory(LocationSync)
endif()

if(WPEFRAMEWORK_PLUGIN_MONITOR)
    add_subdirectory(Monitor)
endif()

if(WPEFRAMEWORK_PLUGIN_NETWORKCONTROL)
    add_subdirectory(NetworkControl)
endif()

if(WPEFRAMEWORK_PLUGIN_OPENCDMI)
    add_subdirectory(OpenCDMi)
endif()

if(WPEFRAMEWORK_PLUGIN_POWER)
    add_subdirectory(Power)
endif()

if(WPEFRAMEWORK_PLUGIN_REMOTECONTROL)
    add_subdirectory(RemoteControl)
endif()

if(WPEFRAMEWORK_PLUGIN_SNAPSHOT)
    add_subdirectory(Snapshot)
endif()

if(WPEFRAMEWORK_PLUGIN_STREAMER)
    add_subdirectory(Streamer)
endif()

if(WPEFRAMEWORK_PLUGIN_SYSTEMDCONNECTOR)
    add_subdirectory(SystemdConnector)
endif()

if(WPEFRAMEWORK_PLUGIN_TIMESYNC)
    add_subdirectory(TimeSync)
endif()

if(WPEFRAMEWORK_PLUGIN_TRACECONTROL)
    add_subdirectory(TraceControl)
endif()

if(WPEFRAMEWORK_PLUGIN_TVCONTROL)
    add_subdirectory(TVControl)
endif(WPEFRAMEWORK_PLUGIN_TVCONTROL)

if(WPEFRAMEWORK_PLUGIN_WEBKITBROWSER)
    add_subdirectory(WebKitBrowser)
endif()

if(WPEFRAMEWORK_PLUGIN_WEBPROXY)
    add_subdirectory(WebProxy)
endif()

if(WPEFRAMEWORK_PLUGIN_WEBSERVER)
    add_subdirectory(WebServer)
endif()

if(WPEFRAMEWORK_PLUGIN_WEBSHELL)
    add_subdirectory(WebShell)
endif()

if(WPEFRAMEWORK_PLUGIN_WIFICONTROL)
    add_subdirectory(WifiControl)
endif()

if(WPEFRAMEWORK_PLUGIN_RTSPCLIENT)
    add_subdirectory(RtspClient)
endif()
