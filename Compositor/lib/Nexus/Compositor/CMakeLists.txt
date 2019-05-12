set(TARGET ${PLATFORM_COMPOSITOR})

message("Setting up ${TARGET} for Nexus platfrom")

find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)

add_library(${TARGET}
        Implementation.cpp
        )

include_directories(${CORE_INCLUDE_DIR})

target_link_libraries(${TARGET}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

if(NEXUS_SERVER_EXTERNAL)
    find_package(NXCLIENT REQUIRED)
    target_link_libraries(${TARGET}
        PRIVATE
            ${NAMESPACE}nexusserver::${NAMESPACE}nexusserver
            NXCLIENT::NXCLIENT
        )
else()
    find_package(NXCLIENT_LOCAL REQUIRED)
    target_compile_definitions(${TARGET}
        PRIVATE
            INCLUDE_NEXUS_SERVER
            )

    target_link_libraries(${TARGET}
        PRIVATE
            ${NAMESPACE}nexusserver::${NAMESPACE}nexusserver
            NXCLIENT_LOCAL::NXCLIENT_LOCAL
            )
endif()

install(TARGETS ${TARGET}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/Compositor
        )
