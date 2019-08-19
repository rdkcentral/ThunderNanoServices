#[[==
  If running a nxserver and nxclient in the same process you need to link libnxclienet_local instead of
  nxclient. Only one client can run as a local client, the IPC mechanism will still be available
  for other clients
#==]]

set(TARGET ${NAMESPACE}nexusserver)

option(PLUGIN_COMPOSITOR_HARDWAREREADY "Manual overrule of the time it takes to initialisize all hardware (s) [0 - UINT16_MAX]." 0)
option(PLUGIN_COMPOSITOR_BOXMODE "Allows for selecting a boxmode (Nexus only).")
option(PLUGIN_COMPOSITOR_GRAPHICS_HEAP_SIZE "Change graphic heap of driver (Nexus only).")
option(PLUGIN_COMPOSITOR_INSTALL_NEXUS_SERVER "Enable to install the server to staging." OFF)
option(NEXUS_SERVER_HAS_EXTENDED_INIT "Enable to support unauthenticated nxclients." ON)
option(NEXUS_SERVER_EXTERNAL "Enable hooking up to an eternally running NxServer instead of running one." OFF)

message("Setup ${TARGET} v${VERSION}...")

find_package(${NAMESPACE}Core REQUIRED)

add_library(${TARGET} STATIC
        NexusServer.cpp)

if (NEXUS_SERVER_EXTERNAL)
    find_package(NXCLIENT REQUIRED)
    target_link_libraries(${TARGET}
        PRIVATE
            ${NAMESPACE}Core::${NAMESPACE}Core
        PUBLIC
            NXCLIENT::NXCLIENT)
    target_compile_definitions(${TARGET} 
        PRIVATE 
            NEXUS_SERVER_EXTERNAL)
else()
    find_package(NXSERVER REQUIRED)
    find_package(NEXUS REQUIRED)
    find_package(NXCLIENT_LOCAL REQUIRED)

    target_include_directories(${TARGET}
        PRIVATE
            ${LIBNXSERVER_INCLUDE_DIR}
    )

    target_link_libraries(${TARGET}
        PRIVATE
            ${NAMESPACE}Core::${NAMESPACE}Core
            NEXUS::NEXUS
            NXSERVER::NXSERVER
        PUBLIC
            NXCLIENT_LOCAL::NXCLIENT_LOCAL)
endif()

set(PUBLIC_HEADERS NexusServer.h)

if (NEXUS_SERVER_HAS_EXTENDED_INIT)
    target_compile_definitions(${TARGET}
        PRIVATE
            NEXUS_SERVER_HAS_EXTENDED_INIT)
endif()

set_target_properties(${TARGET}
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            FRAMEWORK FALSE
            PUBLIC_HEADER "${PUBLIC_HEADERS}"
            VERSION ${VERSION}
            SOVERSION ${VERSION_MAJOR}
            POSITION_INDEPENDENT_CODE ON
)

add_library(${TARGET}::${TARGET} ALIAS ${TARGET})

if (PLUGIN_COMPOSITOR_INSTALL_NEXUS_SERVER)
install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE} COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}     # headers
)

InstallPackageConfig(
        TARGETS ${TARGET}
        DESCRIPTION "A nexus server wrapper.")

InstallCMakeConfig(TARGETS ${TARGET})

endif ()
