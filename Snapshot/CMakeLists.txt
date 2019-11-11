set(PLUGIN_NAME Snapshot)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Tracing REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(BCM_HOST QUIET)
find_package(NEXUS QUIET)
find_package(NXCLIENT QUIET)

add_library(${MODULE_NAME} SHARED
        Module.cpp
        Snapshot.cpp)

target_link_libraries(${MODULE_NAME} 
    PRIVATE 
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Tracing::${NAMESPACE}Tracing
        PNG::PNG)

set_target_properties(${MODULE_NAME}
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

if (NXCLIENT_FOUND AND NEXUS_FOUND)
    target_link_libraries(${MODULE_NAME} 
        PRIVATE 
            NEXUS::NEXUS 
            NXCLIENT::NXCLIENT) 
    target_sources(${MODULE_NAME} 
        PRIVATE 
            Device/NexusCapture.cpp)
elseif (BCM_HOST_FOUND)
    target_link_libraries(${MODULE_NAME} 
        PRIVATE 
            BCM_HOST::BCM_HOST)
    target_sources(${MODULE_NAME} 
        PRIVATE 
            Device/Dispmanx.cpp)
else ()
    message(FATAL_ERROR "There is no graphic backend for Snapshot plugin")
endif ()

install(TARGETS ${MODULE_NAME} 
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
