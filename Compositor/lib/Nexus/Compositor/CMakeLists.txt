set(TARGET ${PLATFORM_COMPOSITOR})

message("Setting up ${TARGET} for Nexus platfrom")

find_package(NXCLIENT REQUIRED)
find_package(NXCLIENT REQUIRED)
find_package(NXCLIENT_LOCAL REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Interfaces REQUIRED)

add_library(${TARGET}
        Implementation.cpp
        )

include_directories(${CORE_INCLUDE_DIR})

target_link_libraries(${TARGET}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Interfaces::${NAMESPACE}Interfaces)

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

if(PLUGIN_COMPOSITOR_NXSERVER)
    target_compile_definitions(${TARGET}
        PRIVATE
            INCLUDE_NEXUS_SERVER
            )

    target_link_libraries(${TARGET}
        PRIVATE
            ${NAMESPACE}nexusserver::${NAMESPACE}nexusserver
            NXCLIENT_LOCAL::NXCLIENT_LOCAL
            )
else()
    target_link_libraries(${TARGET}
        PRIVATE
            NXCLIENT::NXCLIENT
        )
endif()

install(TARGETS ${TARGET}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/Compositor
        )
