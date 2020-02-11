set(PLUGIN_NAME Power)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(PLUGIN_POWER_AUTOSTART true CACHE STRING true)
set(PLUGIN_POWER_GPIOPIN "" CACHE STRING "GPIO pin number")
set(PLUGIN_POWER_GPIOTYPE "" CACHE STRING "GPIO type")
# PLUGIN_POWER_POWER_KEY is for setting the keycode of the key that needs to be configured as power button.
# default value `116` corresponds to KEY_POWER. This value comes from input-event-codes.h
# https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
# If this feature is not needed, set this value to 0
set(PLUGIN_POWER_POWER_KEY 116 CACHE STRING "Key Code for power key. To disable set this to 0")
set(PLUGIN_POWER_CONTROL_CLIENTS true CACHE STRING "Control plugins with IStateControl interface")

find_package(NEXUS REQUIRED)
find_package(NXCLIENT REQUIRED)
find_package(BcmPowerManager REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED 
    Power.cpp
    PowerJsonRpc
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME} 
    PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

target_include_directories( ${MODULE_NAME}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

if (NXCLIENT_FOUND)
    message("Building the Nexus....")
    target_sources(${MODULE_NAME} PRIVATE PowerImplementation/Broadcom/PowerImplementation.cpp)
    target_link_libraries(${MODULE_NAME} 
        PRIVATE
            NEXUS::NEXUS
            NXCLIENT::NXCLIENT)
else()
    message("Building the STUB....")
    target_sources(${MODULE_NAME} PRIVATE PowerImplementation/Stub/PowerImplementation.cpp)
endif ()

target_link_libraries(${MODULE_NAME} 
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

if (BCM_PM_FOUND)
    set(POWER_LIBS -Wl,--whole-archive ${BCM_PM_LIBRARIES} -Wl,--no-whole-archive)

    target_link_libraries(${MODULE_NAME}
        PRIVATE
        ${POWER_LIBS})

endif ()

install(TARGETS ${MODULE_NAME} 
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
