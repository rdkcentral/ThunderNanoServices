set(PLUGIN_NAME BluetoothControl)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(PLUGIN_BLUETOOTH_AUTOSTART true CACHE STRING true)
set(PLUGIN_BLUETOOTH_OOP false CACHE STRING true)

find_package(${NAMESPACE}Bluetooth REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)

add_library(${MODULE_NAME} SHARED 
    BluetoothControl.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME} 
    PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Bluetooth::${NAMESPACE}Bluetooth)
        

install(TARGETS ${MODULE_NAME} 
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
