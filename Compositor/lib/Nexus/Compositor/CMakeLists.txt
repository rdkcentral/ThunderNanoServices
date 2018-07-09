set(TARGET NexusCompositor)

message("Setting up ${TARGET} for Nexus platfrom")

find_package(Nexus REQUIRED)
find_package(Core REQUIRED)
find_package(Plugins REQUIRED)

add_library(${TARGET}
        Nexus.cpp
        )

target_link_libraries(${TARGET}
        PRIVATE
        WPEFramework::Core
        WPEFramework::Plugins
        )

set_target_properties(${TARGET}
        PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        OUTPUT_NAME ${compositor_artefact}
        )

if(WPEFRAMEWORK_PLUGIN_COMPOSITOR_NXSERVER)
    target_compile_definitions(${TARGET}
        PRIVATE
            INCLUDE_NEXUS_SERVER
            )

    target_link_libraries(${TARGET}
        PRIVATE
            nexus::nexusserver
            NEXUS::NXCLIENT_LOCAL
            )
else()
target_link_libraries(${TARGET}
        PRIVATE
            NEXUS::NXCLIENT
        )
endif()

install(TARGETS ${TARGET}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/WPEFramework/Compositor
        )
