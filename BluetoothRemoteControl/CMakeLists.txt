set(PLUGIN_NAME BluetoothRemoteControl)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Bluetooth REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

set(PLUGIN_BLUETOOTHREMOTECONTROL_SUPPORT_ADPCM_HQ true CACHE BOOL "Support adpcm-hq audio profile")
set(PLUGIN_BLUETOOTHREMOTECONTROL_SUPPORT_PCM true CACHE BOOL "Support pcm audio profile")

add_library(${MODULE_NAME} SHARED
    BluetoothRemoteControl.cpp
    BluetoothRemoteControlJsonRpc.cpp
    Administrator.cpp
    T4HDecoders.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ${NAMESPACE}Bluetooth::${NAMESPACE}Bluetooth)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

install(
	DIRECTORY keymaps/
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/${PLUGIN_NAME}
	FILES_MATCHING PATTERN "*.json")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")

write_config(${PLUGIN_NAME})
