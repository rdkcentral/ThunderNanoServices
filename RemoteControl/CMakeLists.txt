set(PLUGIN RemoteControl)
set(ARTEFACT WPEFramework${PLUGIN})

set(WPEFRAMEWORK_PLUGIN_REMOTECONTROL_RFCE_REMOTE_ID "GPSTB" CACHE STRING "User string, used for greenpeak")
set(WPEFRAMEWORK_PLUGIN_REMOTECONTROL_RFCE_MODULE "/lib/modules/misc/gpK5.ko" CACHE STRING "path to kernel module")
set(WPEFRAMEWORK_PLUGIN_REMOTECONTROL_RFCE_NODE_ID 249 CACHE STRING "Node ID for the device to be created")

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR})

message("Setting up ${ARTEFACT}")

find_package(Core REQUIRED)
find_package(Plugins REQUIRED)

add_library(${PLUGIN}
	Module.cpp
	RemoteControl.cpp
	RemoteAdministrator.cpp
)

target_link_libraries(${PLUGIN}
	PRIVATE
	WPEFramework::Core
	WPEFramework::Plugins
)

set_target_properties(${PLUGIN} PROPERTIES
	CXX_STANDARD 11
	CXX_STANDARD_REQUIRED YES
	FRAMEWORK FALSE
	OUTPUT_NAME ${ARTEFACT}
)

find_package(NXServer)
find_package(VirtualInput)
find_package(RF4CE)

if(VirtualInput_FOUND)
	message(STATUS "Including VirtualInput support")
	target_link_libraries(${PLUGIN}
		PRIVATE
			WPEFramework::VirtualInput)
	target_compile_definitions(${PLUGIN}
		PRIVATE
			VIRTUAL_INPUT_ENABLED)
endif()

if (WPEFRAMEWORK_PLUGIN_REMOTECONTROL_DEVINPUT)
	message(STATUS "Including dev input support")
	list(APPEND PLUGIN_DEPENDENCIES )
	target_link_libraries(${PLUGIN}
			PRIVATE
			    udev)
	target_sources(${PLUGIN}
			PRIVATE
			    LinuxDevice.cpp)
endif()

if (NXSERVER_FOUND)
	find_package(Nexus REQUIRED)
	target_link_libraries(${PLUGIN}
		PRIVATE
			NEXUS::NEXUS
			NEXUS::NXCLIENT)
	if (WPEFRAMEWORK_PLUGIN_REMOTECONTROL_IR)
		message(STATUS "Including IR support")
		target_compile_definitions(${PLUGIN}
				PRIVATE
					IR_ENABLED)
		target_sources(${PLUGIN}
				PRIVATE
					IRRemote.cpp)
	endif()
endif()

if(RF4CE_FOUND)
    message(STATUS "Including RF4CE support")
		add_definitions(${RF4CE_DEFINITIONS})
		add_definitions(-DGP_SCHED_EXTERNAL_MAIN)
        include_directories(${RF4CE_INCLUDE_DIRS})
        target_sources(${PLUGIN}
			PRIVATE
			    GreenPeak.cpp)
	target_link_libraries(${PLUGIN}
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

install(TARGETS ${PLUGIN}
	DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/wpeframework/plugins
	COMPONENT ${MODULE_NAME})

include(write_configuration)

write_configuration(${PLUGIN})
