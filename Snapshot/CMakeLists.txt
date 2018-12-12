set(PLUGIN_NAME Snapshot)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION})

message("Setting up ${PLUGIN_NAME}")

add_library(${PLUGIN_NAME} SHARED
        Module.cpp
        Snapshot.cpp)

find_package(PNG REQUIRED)

target_link_libraries(${PLUGIN_NAME} PRIVATE PNG::PNG)

set_target_properties(${PLUGIN_NAME}
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            FRAMEWORK FALSE
            OUTPUT_NAME ${MODULE_NAME})

find_package(BCMHost QUIET)
find_package(NXServer QUIET)

if (NXSERVER_FOUND)
    find_package(Nexus REQUIRED)
    target_link_libraries(${PLUGIN_NAME}
            PRIVATE
                NEXUS::NEXUS
                NEXUS::NXCLIENT)

    target_sources(${PLUGIN_NAME}
            PRIVATE
                Device/NexusCapture.cpp)
elseif (BCM_HOST_FOUND)
    target_link_libraries(${PLUGIN_NAME}
            PRIVATE
                BROADCOM::HOST)

    target_sources(${PLUGIN_NAME}
            PRIVATE
                Device/Dispmanx.cpp)
else ()
    message(FATAL_ERROR "There is no graphic backend for Snapshot plugin")
endif ()


install(TARGETS ${PLUGIN_NAME}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/wpeframework/plugins
        COMPONENT ${MODULE_NAME})

write_config(${PLUGIN_NAME})
