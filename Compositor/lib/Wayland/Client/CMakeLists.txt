set(TARGET compositorclient)

message("Setup ${TARGET} v${VERSION}...")

find_package(WaylandClient REQUIRED)
find_package(EGL REQUIRED)
find_package(WaylandEGL REQUIRED)
find_package(Westeros REQUIRED)
find_package(Core REQUIRED)

add_library(${TARGET} SHARED Westeros.cpp)

target_link_libraries(${TARGET}
            PUBLIC
            WPEFramework::Core
            WESTEROS::CLIENT
            WAYLAND::CLIENT
            WAYLAND::EGL
            EGL::EGL
            )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        DEFINE_SYMBOL WPEFRAMEWORK_WAYLAND_EGL_CLIENT
        )

target_include_directories( ${TARGET}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/${NAMESPACE}/wayland>
        $<INSTALL_INTERFACE:include/${NAMESPACE}>
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        INCLUDES DESTINATION include/${NAMESPACE}/wayland      # headers
)

# For cmake based consumers
include (CMakePackageConfigHelpers)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${TARGET}ConfigVersion.cmake VERSION ${VERSION} COMPATIBILITY SameMajorVersion)
add_library(${NAMESPACE}::WaylandEGLClient ALIAS ${TARGET})
install(EXPORT ${TARGET}Targets FILE ${TARGET}Config.cmake NAMESPACE ${NAMESPACE}:: DESTINATION include/cmake)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}ConfigVersion.cmake DESTINATION include/cmake )

# For non-cmake consumers
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/compositor-client.pc.in"
        "${CMAKE_BINARY_DIR}/compositor-client.pc"
        @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/compositor-client.pc DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")

# add_subdirectory (test)
