set(PLUGIN_NAME Bluetooth)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

message("Setting up ${PLUGIN_NAME}")

find_package(GLIB REQUIRED)
find_package(DBUS REQUIRED)

set(PLUGIN_SOURCES
    Bluetooth.cpp
    BluetoothImplementation.cpp
    Module.cpp)

set(PLUGIN_BLUETOOTH_AUTOSTART true CACHE STRING true)
set(PLUGIN_BLUETOOTH_OOP false CACHE STRING true)

# Library definition section
include_directories(${GLIB_INCLUDE_DIRS} ${DBUS_INCLUDE_DIRS})
add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES})
target_link_libraries(${MODULE_NAME} ${PLUGINS_LIBRARIES} ${GLIB_GIO_LIBRARIES} ${GLIB_LIBRARIES} ${DBUS_LIBRARIES})

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config(${PLUGIN_NAME})
