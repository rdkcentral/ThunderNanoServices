set(TARGET compositorclient)

message("Setup ${TARGET} v${VERSION}...")

find_package(WaylandClient REQUIRED)
find_package(EGL REQUIRED)
find_package(WaylandEGL REQUIRED)
find_package(Westeros REQUIRED)
find_package(Core REQUIRED)

find_package(BCMHost QUIET)
if(BCM_HOST_FOUND)
    set(BCM_HOST_LIBS BROADCOM::HOST)
    list(APPEND PLUGIN_DEFINITIONS "-DBCM_HOST=ON")
endif(BCM_HOST_FOUND)


add_library(${TARGET} SHARED Westeros.cpp)

include_directories(${CORE_INCLUDE_DIR})

#target_compile_definitions(${TARGET} PRIVATE ${PLUGIN_DEFINITIONS})
add_compiler_flags(${TARGET} "${PLUGINS_DEFINITIONS}")

target_link_libraries(${TARGET}
            PUBLIC
            ${CORE_LIBRARIES}
            WESTEROS::CLIENT
            WAYLAND::CLIENT
            WAYLAND::EGL
            EGL::EGL
            ${BCM_HOST_LIBS}
            )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        DEFINE_SYMBOL WPEFRAMEWORK_WAYLAND_EGL_CLIENT
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
