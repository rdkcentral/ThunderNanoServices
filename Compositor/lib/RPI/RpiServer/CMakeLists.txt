set(TARGET rpiserver)
set(NAMESPACE rpi)

message("Setup WPEFramework${TARGET} v${VERSION}...")

find_package(Core REQUIRED)

add_library(${TARGET} STATIC
        RpiServer.cpp
        )

target_link_libraries(${TARGET}
    PRIVATE
        WPEFramework::Core
    )

set(PUBLIC_HEADERS
        RpiServer.h
        )

set_target_properties(${TARGET}
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            FRAMEWORK FALSE
            PUBLIC_HEADER "${PUBLIC_HEADERS}"
            OUTPUT_NAME WPEFramework${TARGET}
            POSITION_INDEPENDENT_CODE ON
        )

target_include_directories( ${TARGET}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/${NAMESPACE}>
        )

add_library(${NAMESPACE}::${TARGET} ALIAS ${TARGET})

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
        "${CMAKE_CURRENT_SOURCE_DIR}/rpiserver.pc.in"
        "${CMAKE_BINARY_DIR}/rpiserver.pc"
        @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/rpiserver.pc DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")
