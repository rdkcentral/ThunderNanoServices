set(PLUGIN Snapshot)
set(ARTEFACT WPEFramework${PLUGIN})

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION})

message("Setting up ${ARTEFACT}")

add_library(${PLUGIN} SHARED
        Module.cpp
        Snapshot.cpp)

find_package(PNG REQUIRED)

target_link_libraries(${PLUGIN} PRIVATE PNG::PNG)

set_target_properties(${PLUGIN}
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            FRAMEWORK FALSE
            OUTPUT_NAME ${ARTEFACT})

find_package(BCMHost QUIET)
find_package(NXServer QUIET)

if (NXSERVER_FOUND)
    find_package(Nexus REQUIRED)
    target_link_libraries(${PLUGIN}
            PRIVATE
                NEXUS::NEXUS
                NEXUS::NXCLIENT)

    target_sources(${PLUGIN}
            PRIVATE
                Device/NexusCapture.cpp)
elseif (BCM_HOST_FOUND)
    target_link_libraries(${PLUGIN}
            PRIVATE
                BROADCOM::HOST)

    target_sources(${PLUGIN}
            PRIVATE
                Device/Dispmanx.cpp)
else ()
    message(FATAL_ERROR "There is no graphic backend for Snapshot plugin")
endif ()


install(TARGETS ${PLUGIN}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/wpeframework/plugins
        COMPONENT ${MODULE_NAME})

write_config(${PLUGIN})
