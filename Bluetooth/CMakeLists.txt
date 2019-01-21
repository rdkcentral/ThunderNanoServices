set(PLUGIN_NAME BluetoothControl)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

message("Setting up ${PLUGIN_NAME}")

find_package(Bluez REQUIRED)
find_package(BCMHost QUIET)
find_package(Nexus QUIET)

set(PLUGIN_SOURCES
    BluetoothControl.cpp
    Module.cpp)

set(PLUGIN_BLUETOOTH_AUTOSTART true CACHE STRING true)
set(PLUGIN_BLUETOOTH_OOP false CACHE STRING true)

# Library definition section
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/drivers" ${BLUEZ_INCLUDE_DIRS})
add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES})
add_compiler_flags(${MODULE_NAME} "${PLUGINS_DEFINITIONS}")
target_link_libraries(${MODULE_NAME} ${PLUGINS_LIBRARIES} ${BLUEZ_LIBRARIES})

if (NEXUS_FOUND)
    target_sources(${MODULE_NAME} PRIVATE drivers/Basic.cpp)
elseif (BCM_HOST_FOUND)
    target_sources(${MODULE_NAME} PRIVATE drivers/BCM43XX.cpp)
else ()
    message(FATAL_ERROR "There is no Bluetooth driver backend for Bluetooth plugin")
endif ()

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config(${PLUGIN_NAME})
