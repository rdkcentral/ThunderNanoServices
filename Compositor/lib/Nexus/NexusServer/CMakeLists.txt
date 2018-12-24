#[[==
  If running a nxserver and nxclient in the same process you need to link libnxclienet_local instead of
  nxclient. Only one client can run as a local client, the IPC mechanism will still be available
  for other clients
#==]]

set(TARGET nexusserver)
set(NAMESPACE nexus)

option(PLUGIN_COMPOSITOR_HARDWAREREADY "Manual overrule of the time it takes to initialisize all hardware (s) [0 - UINT16_MAX]." 0)
option(PLUGIN_COMPOSITOR_BOXMODE "Allows for selecting a boxmode (Nexus only).")
option(PLUGIN_COMPOSITOR_GRAPHICS_HEAP_SIZE "Change graphic heap of driver (Nexus only).")
option(PLUGIN_COMPOSITOR_INSTALL_NEXUS_SERVER "Enable to install the server to staging." OFF)
option(NEXUS_SERVER_HAS_EXTENDED_INIT "Enable to support unauthenticated nxclients." ON)
option(NEXUS_SERVER_EXTERNAL "Enable hooking up to an eternally running NxServer instead of running one." OFF)

message("Setup WPEFramework${TARGET} v${VERSION}...")

find_package(Nexus REQUIRED)
find_package(Core REQUIRED)

add_library(${TARGET} STATIC
        NexusServer.cpp
        )


if (NEXUS_SERVER_EXTERNAL)
    target_link_libraries(${TARGET}
        PRIVATE
            ${CORE_LIBRARIES}
        PUBLIC
            NEXUS::NXCLIENT
    )
    target_compile_definitions(${TARGET} 
        PRIVATE 
            NEXUS_SERVER_EXTERNAL
    )
else()
    find_package(NXServer REQUIRED)
    target_link_libraries(${TARGET}
        PRIVATE
            ${CORE_LIBRARIES}
            NEXUS::NEXUS
            NEXUS::NXSERVER
        PUBLIC
            NEXUS::NXCLIENT_LOCAL
        )
endif()

set(PUBLIC_HEADERS NexusServer.h)

if (NEXUS_SERVER_HAS_EXTENDED_INIT)
    target_compile_definitions(${TARGET}
        PRIVATE
            NEXUS_SERVER_HAS_EXTENDED_INIT
    )
endif()

set_target_properties(${TARGET}
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            FRAMEWORK FALSE
            PUBLIC_HEADER "${PUBLIC_HEADERS}"
            OUTPUT_NAME WPEFramework${TARGET}
            POSITION_INDEPENDENT_CODE ON
        )

add_library(${NAMESPACE}::${TARGET} ALIAS ${TARGET})

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

# For cmake based consumers
include (CMakePackageConfigHelpers)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${TARGET}ConfigVersion.cmake VERSION ${VERSION} COMPATIBILITY SameMajorVersion)
install(EXPORT ${TARGET}Targets FILE ${TARGET}Config.cmake NAMESPACE ${NAMESPACE}:: DESTINATION include/cmake)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}ConfigVersion.cmake DESTINATION include/cmake )

# For non-cmake consumers
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/nexusserver.pc.in"
        "${CMAKE_BINARY_DIR}/nexusserver.pc"
        @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/nexusserver.pc DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")
endif ()
