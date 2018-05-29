set(target_lib_name waylandeglclient)

set(BUILD_SHARED_LIBS ON)

find_package(Wayland REQUIRED)
find_package(EGL REQUIRED)
find_package(WaylandEGL REQUIRED)

if(WAYLAND_FOUND)
    message("-- Wayland support detected")
    include_directories(${WAYLANDSERVER_INCLUDE_DIRS})
    list(APPEND WPEFRAMEWORK_WAYLAND_CLIENT_LIBS ${WAYLAND_LIBRARIES})
    include_directories(${WAYLAND_INCLUDE_DIRS})
endif()

find_package(Westeros)
if(WESTEROS_FOUND)
    message("-- Westeros support detected")
    list(APPEND WPEFRAMEWORK_WAYLAND_CLIENT_SOURCES Westeros.cpp)
    list(APPEND WPEFRAMEWORK_WAYLAND_CLIENT_LIBS
            ${EGL_LIBRARIES}
            ${WAYLAND_EGL_LIBRARIES}
            westeros_simpleshell_client
            westeros_simplebuffer_client
            westeros_compositor)
    include_directories(${WESTEROS_INCLUDE_DIRS})
endif()

install(FILES Client.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/WPEFramework/wayland)

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/wayland-egl-client.pc.in"
        "${CMAKE_BINARY_DIR}/wayland-egl-client.pc"
        @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/wayland-egl-client.pc DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")

# add the waylandeglhelper library
if(WESTEROS_FOUND)
    add_library(${target_lib_name} ${WPEFRAMEWORK_WAYLAND_CLIENT_SOURCES})
    target_link_libraries(${target_lib_name} ${WPEFRAMEWORK_WAYLAND_CLIENT_LIBS})
    set_target_properties(${target_lib_name} PROPERTIES DEFINE_SYMBOL WPEFRAMEWORK_WAYLAND_EGL_CLIENT)
    target_compile_options(${target_lib_name} PRIVATE  ${WAYLAND_EGL_CFLAGS})
    install(TARGETS ${target_lib_name} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif()

add_subdirectory (test)
