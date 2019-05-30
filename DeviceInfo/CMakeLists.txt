set(PLUGIN_NAME DeviceInfo)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

option(PROVIDE_NEXUS_OTP_ID "Let DeviceInfo provide the Nexus OPT ID as device identifier" OFF)

find_package(${NAMESPACE}Plugins REQUIRED)

add_library(${MODULE_NAME} SHARED
    DeviceInfo.cpp
    DeviceInfoJsonRpc.cpp
    Module.cpp)

if(PROVIDE_NEXUS_OTP_ID)
    find_package(NEXUS REQUIRED)
    find_package(NXCLIENT REQUIRED)
    target_compile_definitions(${MODULE_NAME} 
        PRIVATE 
            PROVIDE_NEXUS_OTP_ID)
    target_link_libraries(${MODULE_NAME} 
        PRIVATE
            NEXUS::NEXUS
            NXCLIENT::NXCLIENT)
endif()

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME} 
    PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

install(TARGETS ${MODULE_NAME} 
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
