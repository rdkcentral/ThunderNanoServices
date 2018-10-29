set(PLUGIN_NAME Power)

include(module) # Setup default stuff needed for the cmake framework thingy.

set(WPEFRAMEWORK_PLUGIN_POWER_AUTOSTART true CACHE STRING true)
set(WPEFRAMEWORK_PLUGIN_POWER_GPIOPIN "" CACHE STRING "GPIO pin number")
set(WPEFRAMEWORK_PLUGIN_POWER_GPIOTYPE "" CACHE STRING "GPIO type")

find_package(BcmNxServer REQUIRED)
find_package(BcmPowerManager REQUIRED)

set(PLUGIN_DEFINITIONS)

set(PLUGIN_OPTIONS)

set(PLUGIN_LINK_OPTIONS)

set(PLUGIN_DEPENDENCIES)

set(PLUGIN_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkPlugins}
    ${PLUGIN_DEPENDENCIES})

file(GLOB PLUGIN_INCLUDES *.h)

set(PLUGIN_SOURCES
    Module.cpp
    Power.cpp
    )

set(PLUGIN_DEPENDENCIES_INCLUDE_DIRS . )

if (LIBNXSERVER_FOUND)
    message("Building the Nexus....")
    list(APPEND PLUGIN_SOURCES PowerImplementation/Broadcom/PowerImplementation.cpp)
    list(APPEND PLUGIN_DEPENDENCIES ${LIBNEXUS_LIBRARY} ${LIBNXCLIENT_LIBRARY})
else()
    message("Building the STUB....")
    list(APPEND PLUGIN_SOURCES PowerImplementation/Stub/PowerImplementation.cpp)
endif ()

if (BCM_PM_FOUND)
    set(POWER_LIBS -Wl,--whole-archive ${BCM_PM_LIBRARIES} -Wl,--no-whole-archive)
    list(APPEND PLUGIN_DEPENDENCIES_INCLUDE_DIRS ${BCM_PM_INCLUDE_DIRS})
    list(APPEND PLUGIN_DEPENDENCIES ${POWER_LIBS})
endif ()

set(PLUGIN_INPUT
    ${PLUGIN_SOURCES}
    ${PLUGIN_INCLUDES}
    )

include(default_targets) # This is a regular plugin no need to edit the defaults

write_config( ${PLUGIN_NAME} )
