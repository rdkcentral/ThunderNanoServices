set(PLUGIN_NAME DeviceIdentification)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(NEXUS QUIET)
find_package(BCM_HOST QUIET)

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
    DeviceIdentification.cpp
    Module.cpp)

if(NEXUS_FOUND)
    find_package(NXCLIENT REQUIRED)
    target_link_libraries(${MODULE_NAME} 
        PRIVATE
            NEXUS::NEXUS
            NXCLIENT::NXCLIENT)
    target_sources(${MODULE_NAME} 
        PRIVATE 
            Implementation/Nexus/Nexus.cpp)
elseif (BCM_HOST_FOUND)
    target_sources(${MODULE_NAME}
        PRIVATE
            Implementation/RPI/RPI.cpp)
    target_link_libraries(${MODULE_NAME}
        PRIVATE
            BCM_HOST::BCM_HOST)
else ()
    message(FATAL_ERROR "There is no platform backend for device identifier plugin")
endif()

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME} 
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

target_include_directories(${MODULE_NAME} 
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR})

install(TARGETS ${MODULE_NAME} 
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
