set(PLUGIN_NAME Snapshot)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

message("Setting up ${PLUGIN_NAME}")

find_package(PNG REQUIRED)
find_package(BCMHost QUIET)
find_package(NXServer QUIET)

add_library(${MODULE_NAME} SHARED
        Module.cpp
        Snapshot.cpp)

target_link_libraries(${MODULE_NAME} PRIVATE PNG::PNG)

set_target_properties(${MODULE_NAME}
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            FRAMEWORK FALSE
            OUTPUT_NAME ${MODULE_NAME})

if (NXSERVER_FOUND)
    find_package(Nexus REQUIRED)
    target_link_libraries(${MODULE_NAME} PRIVATE NEXUS::NEXUS NEXUS::NXCLIENT) 
    target_sources(${MODULE_NAME} PRIVATE Device/NexusCapture.cpp)
elseif (BCM_HOST_FOUND)
    target_link_libraries(${MODULE_NAME} PRIVATE BROADCOM::HOST)
    target_sources(${MODULE_NAME} PRIVATE Device/Dispmanx.cpp)
else ()
    message(FATAL_ERROR "There is no graphic backend for Snapshot plugin")
endif ()

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config(${PLUGIN_NAME})
