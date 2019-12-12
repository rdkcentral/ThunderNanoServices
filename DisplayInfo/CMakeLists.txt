set(PLUGIN_NAME DisplayInfo)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(BCM_HOST QUIET)
find_package(NEXUS QUIET)
find_package(NXCLIENT QUIET)

add_library(${MODULE_NAME} SHARED
    DisplayInfo.cpp
    DisplayInfoJsonRpc.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

if (NXCLIENT_FOUND AND NEXUS_FOUND)
    target_sources(${MODULE_NAME}
        PRIVATE
            Nexus/PlatformImplementation.cpp)
    target_link_libraries(${MODULE_NAME}
        PRIVATE
            NEXUS::NEXUS
            NXCLIENT::NXCLIENT)
elseif (BCM_HOST_FOUND)
    target_sources(${MODULE_NAME}
        PRIVATE
            RPI/PlatformImplementation.cpp)
    target_link_libraries(${MODULE_NAME}
        PRIVATE
            BCM_HOST::BCM_HOST)
else ()
    message(FATAL_ERROR "There is no graphic backend for display info plugin")
endif ()

install(TARGETS ${MODULE_NAME} 
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
