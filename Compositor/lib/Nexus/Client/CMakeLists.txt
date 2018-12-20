set(TARGET compositorclient)

message("Setup ${TARGET} v${VERSION}...")

find_package(EGL REQUIRED)
find_package(Core REQUIRED)
find_package(VirtualInput REQUIRED)
find_package(Interfaces REQUIRED)

add_library(${TARGET} SHARED Implementation.cpp)

target_link_libraries(${TARGET}
            PUBLIC
            ${CORE_LIBRARIES}
            ${VIRTUALINPUT_LIBRARIES}
            ${INTERFACES_LIBRARIES}
            EGL::EGL
            )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}"
        DEFINE_SYMBOL WPEFRAMEWORK_WAYLAND_EGL_CLIENT
        )

add_definitions(${EGL_DEFINITIONS})

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
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
