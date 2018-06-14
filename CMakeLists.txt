cmake_minimum_required(VERSION 2.8)

# WPEFRAMEWORK_ROOT acts as the root directory for this project, in case it is embedded in a larger, surrounding project.
# COMPONENT_NAME is used to group multiple install targets (one per module).
# MODULE_NAME is used as a replacement for PROJECT_NAME, as using project(x) and PROJECT_NAME seems to interfere with
# Yocto builds.
# Be careful to correctly set MODULE_NAME, as it is also used in the uninstall.cmake.in files for naming the uninstall manifest.
# Additionally, install targets are linked to the MODULE_NAME and COMPONENT_NAME.

set(WPEFRAMEWORK_PLUGINS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(COMPONENT_NAME WPEFrameworkPlugins)

option(WPEFRAMEWORK_VERBOSE_BUILD "Verbose build output" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# This is where all the cmake configs are installed.
set(CMAKE_PREFIX_PATH ${CMAKE_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/cmake ${CMAKE_PREFIX_PATH})
set(NAMESPACE WPEFramework CACHE STRING "cmake namespace definition")
set(CMAKE_MODULE_PATH
        "${WPEFRAMEWORK_PLUGINS_ROOT}/cmake"
        "${CMAKE_SYSROOT}/${CMAKE_INSTALL_PREFIX}/include/WPEFramework/cmake"
        ${CMAKE_MODULE_PATH})

if(CMAKE_CROSSCOMPILING)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/cmake")
    foreach(searchPath ${CMAKE_FIND_ROOT_PATH})
        if(EXISTS "${searchPath}/usr/include/WPEFramework/cmake/config.cmake")
            include(${searchPath}/usr/include/WPEFramework/cmake/config.cmake)
        endif(EXISTS "${searchPath}/usr/include/WPEFramework/cmake/config.cmake")
    endforeach()
else(CMAKE_CROSSCOMPILING)
    if(EXISTS "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/cmake/config.cmake")
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/cmake")
        include(${CMAKE_INSTALL_PREFIX}/include/WPEFramework/cmake/config.cmake)
    else()
        if(EXISTS "${WPEFRAMEWORK_ROOT}/Source/cmake/config.cmake")
            list(APPEND CMAKE_MODULE_PATH "${WPEFRAMEWORK_ROOT}/Source/cmake")
            include(${WPEFRAMEWORK_ROOT}/Source/cmake/config.cmake)
        endif()
    endif()
endif(CMAKE_CROSSCOMPILING)

include(list_to_string)

list_to_string(CMAKE_MODULE_PATH CMAKE_MODULE_PATH_STRING)
if (WPEFRAMEWORK_VERBOSE_BUILD)
    message(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH_STRING})
endif()

# This is to ensure that if CMAKE_INSTALL_PREFIX is set to a non-default location, pkgconfig can find the .pc file
if (NOT CMAKE_CROSSCOMPILING)
    set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
    set(CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})
endif ()

# Git (and its revision)
find_package(Git QUIET) # if we don't find git or FindGit.cmake is not on the system we ignore it.
set(VERSION_REVISION "0000000")
if(GIT_FOUND) # GIT revision
    include(GetGitRevisionDescription)
    get_git_short_revision(GIT_SHA1)
    message(STATUS "GIT revision ${GIT_SHA1}")
    set(VERSION_REVISION ${GIT_SHA1})
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(CONFIG_DIR "debug" CACHE STRING "Build config diurectory" FORCE)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set(CONFIG_DIR "release" CACHE STRING "Build config directory" FORCE)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "MinSizeRel")
    set(CONFIG_DIR "releaseMinSize" CACHE STRING "Build config directory" FORCE)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
    set(CONFIG_DIR "releaseMinSize" CACHE STRING "Build config directory" FORCE)
else()
    message(FATAL_ERROR "Invalid build type: " ${CMAKE_BUILD_TYPE})
endif()

if (WPEFRAMEWORK_VERBOSE_BUILD)
    message(STATUS "Build config directory: " ${CMAKE_BINARY_DIR})
endif()

if (NOT "${WPEFRAMEWORK_ROOT}" STREQUAL "")
    set(OUTPUT_BASE_DIR ${WPEFRAMEWORK_ROOT}/output)
else()
    set(OUTPUT_BASE_DIR ${CMAKE_SOURCE_DIR}/output)
endif()
if (WPEFRAMEWORK_VERBOSE_BUILD)
    message(STATUS "Output base directory: " ${OUTPUT_BASE_DIR})
endif()

find_package(WPEFrameworkPlugins QUIET)

add_custom_target(install-component-WPEFrameworkPlugins)
add_custom_target(uninstall-component-WPEFrameworkPlugins)

if(CLION_ENVIRONMENT)
    add_dependencies(install-component-WPEFrameworkPlugins install-component-WPEFramework)
    add_dependencies(uninstall-component-WPEFrameworkPlugins uninstall-component-WPEFramework)
endif()

option(WPEFRAMEWORK_PLUGIN_BLUETOOTH "Include Bluetooth plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_COMMANDER "Include Commander plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_COMPOSITOR "Include Compsitor plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_DEVICEINFO "Include DeviceInfo plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_DHCPSERVER "Include DHCPServer plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_LOCATIONSYNC "Include LocationSync plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_MONITOR "Include Monitor plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_NETWORKCONTROL "Include NetworkControlplugin" OFF)
option(WPEFRAMEWORK_PLUGIN_OPENCDMI "Include OpenCDMi plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_REMOTECONTROL "Include RemoteControl plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_SNAPSHOT "Include Snapshot plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_TIMESYNC "Include TimeSync plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_TRACECONTROL "Include TraceControl plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_WEBKITBROWSER "Include WebKitBrowser plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_WEBPROXY "Include WebProxy plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_WEBSERVER "Include WebServer plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_WEBSHELL "Include WebShell plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_TVCONTROL "Include TVControl plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_WIFICONTROL "Include WifiControl plugin" OFF)
option(WPEFRAMEWORK_PLUGIN_NETFLIX "Has Netflix plugin" OFF)

include_directories(${WPEFRAMEWORK_PLUGINS_INCLUDE_DIRS})

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

if(WPEFRAMEWORK_PLUGIN_DHCPSERVER)
    add_subdirectory(DHCPServer)
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

if(WPEFRAMEWORK_PLUGIN_REMOTECONTROL)
    add_subdirectory(RemoteControl)
endif()

if(WPEFRAMEWORK_PLUGIN_SNAPSHOT)
    add_subdirectory(Snapshot)
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
