set(PLUGIN_NAME RemoteControl)
set(MODULE_NAME ${NAMESPACE}${PLUGIN})

option(WPEFRAMEWORK_PLUGIN_REMOTECONTROL_RFCE "Enable RF4CE functionality." ON)

set(WPEFRAMEWORK_PLUGIN_REMOTECONTROL_RFCE_REMOTE_ID "GPSTB" CACHE STRING "User string, used for greenpeak")
set(WPEFRAMEWORK_PLUGIN_REMOTECONTROL_RFCE_MODULE "/lib/modules/misc/gpK5.ko" CACHE STRING "path to kernel module")
set(WPEFRAMEWORK_PLUGIN_REMOTECONTROL_RFCE_NODE_ID 249 CACHE STRING "Node ID for the device to be created")

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR})

message("Setting up ${ARTEFACT}")

add_library(${MODULE_NAME} Module.cpp RemoteControl.cpp RemoteAdministrator.cpp)
target_link_libraries(${MODULE_NAME} PRIVATE ${PLUGINS_LIBRARIES})
set_target_properties(${MODULE_NAME} PROPERTIES 
        CXX_STANDARD 11
	CXX_STANDARD_REQUIRED YES
	FRAMEWORK FALSE
	OUTPUT_NAME ${MODULE_NAME}
)

find_package(NXServer QUIET)
find_package(VirtualInput QUIET)

if(WPEFRAMEWORK_PLUGIN_REMOTECONTROL_RFCE)
	find_package(RF4CE QUIET)
endif()

if(VirtualInput_FOUND)
	message(STATUS "Including VirtualInput support")
	target_link_libraries(${MODULE_NAME}
		PRIVATE
			WPEFramework::VirtualInput)
	target_compile_definitions(${MODULE_NAME}
		PRIVATE
			VIRTUAL_INPUT_ENABLED)
endif()

if (WPEFRAMEWORK_PLUGIN_REMOTECONTROL_DEVINPUT)
	message(STATUS "Including dev input support")
	list(APPEND PLUGIN_DEPENDENCIES )
	target_link_libraries(${MODULE_NAME}
			PRIVATE
			    udev)
	target_sources(${MODULE_NAME}
			PRIVATE
			    LinuxDevice.cpp)
endif()

if (NXSERVER_FOUND)
	find_package(Nexus REQUIRED)
	target_link_libraries(${MODULE_NAME}
		PRIVATE
			NEXUS::NEXUS
			NEXUS::NXCLIENT)
	if (WPEFRAMEWORK_PLUGIN_REMOTECONTROL_IR)
		message(STATUS "Including IR support")
		target_compile_definitions(${MODULE_NAME}
				PRIVATE
					IR_ENABLED)
		target_sources(${MODULE_NAME}
				PRIVATE
					IRRemote.cpp)
	endif()
endif()

if(RF4CE_FOUND)
    message(STATUS "Including RF4CE support")
		add_definitions(${RF4CE_DEFINITIONS})
		add_definitions(-DGP_SCHED_EXTERNAL_MAIN)
        include_directories(${RF4CE_INCLUDE_DIRS})
        target_sources(${MODULE_NAME}
			PRIVATE
			    GreenPeak.cpp)
	target_link_libraries(${MODULE_NAME}
			PRIVATE
			    ${RF4CE_LIBRARIES})
endif()

install(
	FILES linux-windows.json
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share/WPEFramework/RemoteControl
	COMPONENT ${MODULE_NAME})

install(
	FILES web-remote.json
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share/WPEFramework/RemoteControl
	COMPONENT ${MODULE_NAME})

install(TARGETS ${MODULE_NAME}
	DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/wpeframework/plugins
	COMPONENT ${MODULE_NAME})

write_config(${PLUGIN})
