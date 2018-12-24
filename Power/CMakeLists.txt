set(PLUGIN_NAME Power)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

message("Setting up ${PLUGIN_NAME}")

set(PLUGIN_POWER_AUTOSTART true CACHE STRING true)
set(PLUGIN_POWER_GPIOPIN "" CACHE STRING "GPIO pin number")
set(PLUGIN_POWER_GPIOTYPE "" CACHE STRING "GPIO type")

find_package(BcmNxServer REQUIRED)
find_package(BcmPowerManager REQUIRED)

set(PLUGIN_SOURCES
    Power.cpp
    Module.cpp)

set (PLUGIN_LIBS ${PLUGINS_LIBRARIES})

if (LIBNXSERVER_FOUND)
    message("Building the Nexus....")
    list(APPEND PLUGIN_SOURCES PowerImplementation/Broadcom/PowerImplementation.cpp)
    list(APPEND PLUGIN_LIBS ${LIBNEXUS_LIBRARY} ${LIBNXCLIENT_LIBRARY})
else()
    message("Building the STUB....")
    list(APPEND PLUGIN_SOURCES PowerImplementation/Stub/PowerImplementation.cpp)
endif ()

if (BCM_PM_FOUND)
    set(POWER_LIBS -Wl,--whole-archive ${BCM_PM_LIBRARIES} -Wl,--no-whole-archive)
    list(APPEND PLUGIN_INCLUDE_DIRS ${BCM_PM_INCLUDE_DIRS})
    list(APPEND PLUGIN_LIBS ${POWER_LIBS})
endif ()

# Library definition section
include_directories(${PLUGIN_INCLUDE_DIRS})
add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES})
target_link_libraries(${MODULE_NAME} ${PLUGIN_LIBS})

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config(${PLUGIN_NAME})
