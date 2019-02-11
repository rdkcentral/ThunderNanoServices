set(PLUGIN_NAME BluetoothControl)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(PLUGIN_BLUETOOTH_AUTOSTART true CACHE STRING true)
set(PLUGIN_BLUETOOTH_OOP false CACHE STRING true)



find_package(Bluez REQUIRED)
find_package(BCM_HOST QUIET)
find_package(NEXUS QUIET)

find_package(${NAMESPACE}Plugins REQUIRED)

add_library(${MODULE_NAME} SHARED 
    BluetoothControl.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_include_directories(${MODULE_NAME} 
    PRIVATE 
        "${CMAKE_CURRENT_SOURCE_DIR}/drivers" 
        ${BLUEZ_INCLUDE_DIRS})

target_link_libraries(${MODULE_NAME} 
    PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${BLUEZ_LIBRARIES})

if (NEXUS_FOUND)
    target_sources(${MODULE_NAME} PRIVATE drivers/Basic.cpp)
elseif (BCM_HOST_FOUND)
    target_sources(${MODULE_NAME} PRIVATE drivers/BCM43XX.cpp)
else ()
    message(FATAL_ERROR "There is no Bluetooth driver backend for Bluetooth plugin")
endif ()

install(TARGETS ${MODULE_NAME} 
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
