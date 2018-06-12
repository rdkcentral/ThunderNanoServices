set(TARGET waylandtestclient)

message("Setup ${TARGET} v${VERSION}...")

add_executable(${TARGET} WaylandClient.cpp)

find_package(GLESv2 REQUIRED)

target_link_libraries(${TARGET}
        PRIVATE
        WPEFramework::WaylandEGLClient
        EGL::GLESV2
        )

set_target_properties(${TARGET} PROPERTIES
        DEFINE_SYMBOL WPEFRAMEWORK_WAYLAND_CLIENT_APP)

install(TARGETS ${TARGET} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
