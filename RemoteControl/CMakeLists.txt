set(PLUGIN_NAME RemoteControl)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

option(PLUGIN_REMOTECONTROL_RFCE "Enable RF4CE functionality." ON)

set(PLUGIN_REMOTECONTROL_RFCE_REMOTE_ID "GPSTB" CACHE STRING "User string, used for greenpeak")
set(PLUGIN_REMOTECONTROL_RFCE_MODULE "/lib/modules/misc/gpK5.ko" CACHE STRING "path to kernel module")
set(PLUGIN_REMOTECONTROL_RFCE_NODE_ID 249 CACHE STRING "Node ID for the device to be created")

message("Setting up ${MODULE_NAME}")

find_package(NXServer QUIET)

add_library(${MODULE_NAME} Module.cpp RemoteControl.cpp RemoteAdministrator.cpp)

if (NXSERVER_FOUND)
    find_package(Nexus REQUIRED)
    if (PLUGIN_REMOTECONTROL_IR)
        message(STATUS "Including IR support")
        target_compile_definitions(${MODULE_NAME} PRIVATE IR_ENABLED)
        target_sources(${MODULE_NAME} PRIVATE IRRemote.cpp)
        target_link_libraries(${MODULE_NAME} PRIVATE NEXUS::NEXUS NEXUS::NXCLIENT)
    endif()
endif()

if(PLUGIN_REMOTECONTROL_RFCE)
    find_package(RF4CE QUIET)
    if (RF4CE_FOUND)
        message(STATUS "Including RF4CE support")
        add_definitions(${RF4CE_DEFINITIONS})
        add_definitions(-DGP_SCHED_EXTERNAL_MAIN)
        include_directories(${RF4CE_INCLUDE_DIRS})
        target_sources(${MODULE_NAME} PRIVATE GreenPeak.cpp)
        target_link_libraries(${MODULE_NAME} PRIVATE ${RF4CE_LIBRARIES})
    endif()
endif()

if (PLUGIN_REMOTECONTROL_DEVINPUT)
    message(STATUS "Including dev input support")
    target_sources(${MODULE_NAME} PRIVATE LinuxDevice.cpp)
    target_link_libraries(${MODULE_NAME} PRIVATE udev)
endif()

target_link_libraries(${MODULE_NAME} PRIVATE ${PLUGINS_LIBRARIES})
set_target_properties(${MODULE_NAME} PROPERTIES 
        CXX_STANDARD 11
	CXX_STANDARD_REQUIRED YES
	FRAMEWORK FALSE
	OUTPUT_NAME ${MODULE_NAME}
)

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)


install(
	FILES linux-windows.json
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/RemoteControl
	COMPONENT ${MODULE_NAME})

install(
	FILES web-remote.json
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/RemoteControl
	COMPONENT ${MODULE_NAME})

write_config(${PLUGIN_NAME})
