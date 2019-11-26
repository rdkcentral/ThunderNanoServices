set(PLUGIN_NAME Power)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(PLUGIN_POWER_AUTOSTART true CACHE STRING true)
set(PLUGIN_POWER_GPIOPIN "" CACHE STRING "GPIO pin number")
set(PLUGIN_POWER_GPIOTYPE "" CACHE STRING "GPIO type")

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
