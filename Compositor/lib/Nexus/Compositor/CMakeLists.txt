set(TARGET ${PLATFORM_COMPOSITOR})

message("Setting up ${TARGET} for Nexus platfrom")

find_package(Nexus REQUIRED)
find_package(Core REQUIRED)
find_package(Plugins REQUIRED)
find_package(Interfaces REQUIRED)

add_library(${TARGET}
        Implementation.cpp
        )

include_directories(${CORE_INCLUDE_DIR})

target_link_libraries(${TARGET}
        PRIVATE
        ${CORE_LIBRARIES}
        ${PLUGINS_LIBRARIES}
        ${INTERFACES_LIBRARIES}
        )

set_target_properties(${TARGET}
        PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        )

if(PLUGIN_COMPOSITOR_NXSERVER)
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
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/Compositor
        )
