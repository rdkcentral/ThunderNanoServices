set(PLUGIN_NAME DHCPServer)

include(module) # Setup default stuff needed for the cmake framework thingy.


set(MODULE_NAME WPEFramework${PLUGIN_NAME})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")

message("Setting up ${MODULE_NAME}")

set(PLUGIN_DEPENDENCIES_INCLUDE_DIRS)

set(PLUGIN_DEFINITIONS
    PLUGIN_DHCPSERVER)

set(PLUGIN_INCLUDE_DIRS
    ${WPEFRAMEWORK_INCLUDE_DIRS}
    ${PLUGIN_DEPENDENCIES_INCLUDE_DIRS})

set(PLUGIN_OPTIONS)

set(PLUGIN_DEPENDENCIES)

set(PLUGIN_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkPlugins}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkCore}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkTracing}
    ${PLUGIN_DEPENDENCIES})

file(GLOB PLUGIN_INCLUDES *.h)

set(PLUGIN_SOURCES
    Module.cpp
    DHCPServer.cpp
    DHCPServerImplementation.cpp
)

set(PLUGIN_INPUT
    ${PLUGIN_SOURCES}
    ${PLUGIN_INCLUDES})

#if (WPEFRAMEWORK_VERBOSE_BUILD)
    display_list("Defines                     : " ${PLUGIN_DEFINITIONS} )
    display_list("Compiler options            : " ${PLUGIN_OPTIONS} )
    display_list("Source files                : " ${PLUGIN_SOURCES} )
    display_list("Include files               : " ${PLUGIN_INCLUDES} )
    display_list("Include dirs                : " ${PLUGIN_INCLUDE_DIRS} )
    display_list("Link libs                   : " ${PLUGIN_LIBS} )
    display_list("Linker options              : " ${PLUGIN_LINK_OPTIONS} )
    display_list("Dependencies                : " ${PLUGIN_DEPENDENCIES} )
#endif()

add_library(${MODULE_NAME} SHARED ${PLUGIN_INPUT})
target_compile_definitions(${MODULE_NAME} PRIVATE ${PLUGIN_DEFINITIONS})
target_include_directories(${MODULE_NAME} PRIVATE ${PLUGIN_INCLUDE_DIRS})
set_target_properties(${MODULE_NAME} PROPERTIES DEFINE_SYMBOL WPEFRAMEWORK_PLATFORM_PLUGIN)
target_link_libraries(${MODULE_NAME} ${PLUGIN_LIBS})
list_to_string(PLUGIN_LINK_OPTIONS PLUGIN_LINK_OPTIONS_STRING)
if (NOT "${PLUGIN_LINK_OPTIONS_STRING}" STREQUAL "")
    set_target_properties(${MODULE_NAME} PROPERTIES LINK_FLAGS "${PLUGIN_LINK_OPTIONS_STRING}")
endif()
set_target_properties(${MODULE_NAME} PROPERTIES OUTPUT_NAME ${MODULE_NAME})

setup_target_properties_library(${MODULE_NAME})

if (WPEFRAMEWORK_VERBOSE_BUILD)
    show_target_properties(${MODULE_NAME})
endif()

# uninstall target
configure_file(
    ${WPEFRAMEWORK_PLUGINS_ROOT}/cmake/uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake
    IMMEDIATE @ONLY)

add_custom_target(uninstall_${MODULE_NAME}
    COMMAND ${CMAKE_COMMAND} -DCOMPONENT=${MODULE_NAME} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake
    DEPENDS ${MODULE_NAME}
    COMMENT "Uninstalling ${MODULE_NAME}")

add_custom_target(install_${MODULE_NAME}
    COMMAND ${CMAKE_COMMAND} -DCOMPONENT=${MODULE_NAME} -P ${CMAKE_BINARY_DIR}/cmake_install.cmake
    DEPENDS ${MODULE_NAME}
    COMMENT "Installing ${MODULE_NAME}")

add_dependencies(install-component-${COMPONENT_NAME} install_${MODULE_NAME})

add_dependencies(uninstall-component-${COMPONENT_NAME} uninstall_${MODULE_NAME})

install(
    TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/wpeframework/plugins
    COMPONENT ${MODULE_NAME})

# default configuration
map()
    kv(locator lib${MODULE_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
    kv(classname ${PLUGIN_NAME})
    kv(autostart false)
    kv(outofprocess true)
    key(configuration)
    map()
        key(events)
        kv("name" "wpeserver")
        key(servers)
        map()
            kv(interface eth0)
            kv(poolstart 100)
            kv(poolsize 50)
            kv(active false)
        end()
        map()
            kv(interface wlan0)
            kv(poolstart 100)
            kv(poolsize 50)
            kv(active false)
        end()
    end()
end()
ans(PLUGIN_CONFIG)

json_write("${CMAKE_CURRENT_LIST_DIR}/${PLUGIN_NAME}.json" ${PLUGIN_CONFIG})
install(
    FILES ${PLUGIN_NAME}.json
    DESTINATION ${CMAKE_INSTALL_PREFIX}/../etc/WPEFramework/plugins/
    COMPONENT ${MODULE_NAME})
