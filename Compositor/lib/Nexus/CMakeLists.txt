set(target_name platformplugin)

message("Setting up ${target_name} for Nexus platfrom")

if(CMAKE_CROSSCOMPILING)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/cmake")
    foreach(searchPath ${CMAKE_FIND_ROOT_PATH})
        if(EXISTS "${searchPath}/usr/include/WPEFramework/cmake/config.cmake")
            include(${searchPath}/usr/include/WPEFramework/cmake/config.cmake)
        endif(EXISTS "${searchPath}/usr/include/WPEFramework/cmake/config.cmake")
    endforeach()
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/cmake")
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

set(WPEFRAMEWORK_PLATFORM_PLUGIN_LIBS
        WPEFrameworkPlugins
        )

find_package(WPEFramework REQUIRED)
find_package(BcmNxServer REQUIRED)

include_directories(${WPEFRAMEWORK_INCLUDE_DIRS})
include_directories(${LIBNXSERVER_INCLUDE_DIRS})

set(WPEFRAMEWORK_PLATFORM_PLUGIN_SOURCES Nexus.cpp)
list(APPEND WPEFRAMEWORK_PLATFORM_PLUGIN_LIBS ${LIBNXSERVER_LIBRARIES})

# add the platformplugin library
add_library(${target_name} ${WPEFRAMEWORK_PLATFORM_PLUGIN_SOURCES})
target_link_libraries(${target_name} ${WPEFRAMEWORK_PLATFORM_PLUGIN_LIBS})
set_target_properties(${target_name} PROPERTIES DEFINE_SYMBOL WPEFRAMEWORK_PLATFORM_PLUGIN)

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

if (WPEFRAMEWORK_VERBOSE_BUILD)
    include(show_target_properties)
    show_target_properties(${target_name})
endif()
