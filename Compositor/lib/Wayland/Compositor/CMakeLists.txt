set(target_name platformplugin)

set(WPEFRAMEWORK_PLATFORM_PLUGIN_LIBS WPEFrameworkPlugins)
set(BUILD_SHARED_LIBS ON)

option(WPEFRAMEWORK_PLUGIN_COMPOSITOR_VIRTUALINPUT "Enable WPEFramework virtual input support. [ON, OFF]." OFF)
list(APPEND CMAKE_MODULE_PATH "${WPEFRAMEWORK_PLUGINS_ROOT}/cmake")

if(CMAKE_CROSSCOMPILING)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/cmake")
    foreach(searchPath ${CMAKE_FIND_ROOT_PATH})
        if(EXISTS "${searchPath}/usr/include/WPEFramework/cmake/config.cmake")
            include(${searchPath}/usr/include/WPEFramework/cmake/config.cmake)
        endif(EXISTS "${searchPath}/usr/include/WPEFramework/cmake/config.cmake")
    endforeach()
else(CMAKE_CROSSCOMPILING)
    if(EXISTS "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/cmake/config.cmake")
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/cmake")
        include(${CMAKE_INSTALL_PREFIX}/include/WPEFramework/cmake/config.cmake)
    else()
        if(EXISTS "${WPEFRAMEWORK_ROOT}/Source/cmake/config.cmake")
            list(APPEND CMAKE_MODULE_PATH "${WPEFRAMEWORK_ROOT}/Source/cmake")
            include(${WPEFRAMEWORK_ROOT}/Source/cmake/config.cmake)
        endif()
    endif()
endif(CMAKE_CROSSCOMPILING)

find_package(WaylandServer REQUIRED)
find_package(Westeros REQUIRED)
find_package(WPEFramework REQUIRED)


include_directories(${WESTEROS_INCLUDE_DIRS})
include_directories(${WAYLANDSERVER_INCLUDE_DIRS})
include_directories(${WPEFRAMEWORK_INCLUDE_DIRS})

set(WPEFRAMEWORK_PLATFORM_PLUGIN_SOURCES Wayland.cpp)
list(APPEND WPEFRAMEWORK_PLATFORM_PLUGIN_LIBS ${WAYLANDSERVER_LIBRARIES} waylandeglclient)

if(WESTEROS_FOUND)
    find_package(WaylandEGL REQUIRED)
    find_package(EGL REQUIRED)
    list(APPEND WPEFRAMEWORK_PLATFORM_PLUGIN_SOURCES Westeros.cpp)
    list(APPEND WPEFRAMEWORK_PLATFORM_PLUGIN_LIBS ${WESTEROS_LIBRARIES})
endif(WESTEROS_FOUND)

if(WPEFRAMEWORK_PLUGIN_COMPOSITOR_VIRTUALINPUT)
    find_package(LibVirtualKeyboard REQUIRED)
    list(APPEND WPEFRAMEWORK_PLATFORM_PLUGIN_LIBS ${LIBVIRTUAL_KEYBOARD_LIBRARIES})
    include_directories(${LIBVIRTUAL_KEYBOARD_INCLUDE_DIRS})
    add_definitions(-DUSE_WPEFRAMEWORK_VIRTUAL_INPUT=1)
endif(WPEFRAMEWORK_PLUGIN_COMPOSITOR_VIRTUALINPUT)

# add the platformplugin library
add_library(${target_name} ${WPEFRAMEWORK_PLATFORM_PLUGIN_SOURCES})
target_link_libraries(${target_name} ${WPEFRAMEWORK_PLATFORM_PLUGIN_LIBS})
set_target_properties(${target_name} PROPERTIES DEFINE_SYMBOL WPEFRAMEWORK_PLATFORM_PLUGIN)
target_compile_options(${target_name} PRIVATE  ${WAYLAND_EGL_CFLAGS})

# uninstall target
configure_file(
    ${WPEFRAMEWORK_PLUGINS_ROOT}/cmake/uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake
    IMMEDIATE @ONLY)

add_custom_target(uninstall_${target_name}
    COMMAND ${CMAKE_COMMAND} -DCOMPONENT=${target_name} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake
    DEPENDS ${target_name}
    COMMENT "Uninstalling ${target_name}")

add_custom_target(install_${target_name}
    COMMAND ${CMAKE_COMMAND} -DCOMPONENT=${target_name} -P ${CMAKE_BINARY_DIR}/cmake_install.cmake
    DEPENDS ${target_name}
    COMMENT "Installing ${target_name}")

add_dependencies(install-component-${COMPONENT_NAME} install_${target_name})

add_dependencies(uninstall-component-${COMPONENT_NAME} uninstall_${target_name})

install(
    TARGETS ${target_name}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/WPEFramework/Compositor
    COMPONENT ${MODULE_NAME})

