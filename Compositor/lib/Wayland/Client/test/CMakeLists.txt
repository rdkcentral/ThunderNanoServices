set(target_app_name waylandtestclient)
 
set(WPEFRAMEWORK_WAYLAND_CLIENT_APP ${target_lib_name} WPEFrameworkCore )

find_package(GLESv2 REQUIRED)

if(GLESV2_FOUND)
    message("-- GLESv2 support detected")
    include_directories(${GLESV2_INCLUDE_DIRS})
    list(APPEND WPEFRAMEWORK_WAYLAND_CLIENT_APP ${GLESV2_LIBRARIES})
endif(GLESV2_FOUND)

add_executable(${target_app_name} WaylandClient.cpp)
target_link_libraries(${target_app_name} ${WPEFRAMEWORK_WAYLAND_CLIENT_APP})
set_target_properties(${target_app_name} PROPERTIES DEFINE_SYMBOL WPEFRAMEWORK_WAYLAND_CLIENT_APP)
target_compile_options(${target_app_name} PRIVATE  ${GLESV2_CFLAGS})
install(TARGETS ${target_app_name} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
