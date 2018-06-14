set(PLUGIN_NAME Bluetooth)

set(WPEFRAMEWORK_PLUGIN_BLUETOOTH_AUTOSTART true CACHE STRING true)
set(WPEFRAMEWORK_PLUGIN_BLUETOOTH_OOP false CACHE STRING true)

include(module) # Setup default stuff needed for the cmake framework thingy.

find_package(GLIB REQUIRED)
find_package(DBUS REQUIRED)

set(PLUGIN_DEFINITIONS)

set(PLUGIN_OPTIONS)
set(PLUGIN_LINK_OPTIONS -lglib-2.0 -ldbus-1 -lgio-2.0)

set(PLUGIN_SOURCES
        Module.cpp
        Bluetooth.cpp
        BluetoothImplementation.cpp)


set(PLUGIN_INPUT
    ${PLUGIN_SOURCES}
    ${PLUGIN_INCLUDES})

list(APPEND PLUGIN_DEPENDENCIES_INCLUDE_DIRS ${WPEFRAMEWORK_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS} ${DBUS_INCLUDE_DIRS} ${WPE_WEBKIT_INCLUDE_DIRS})
list(APPEND PLUGIN_DEPENDENCIES ${GLIB_GIO_LIBRARIES} ${GLIB_LIBRARIES} ${DBUS_LIBRARIES})

include(default_targets) # This is a regular plugin no need to edit the defaults

write_config(${PLUGIN_NAME})
