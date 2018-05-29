# WPEFRAMEWORK_ROOT acts as the root directory for this project, in case it is embedded in a larger, surrounding project.
# COMPONENT_NAME is used to group multiple install targets (one per module).
# MODULE_NAME is used as a replacement for PROJECT_NAME, as using project(x) and PROJECT_NAME seems to interfere with
# Yocto builds. In some cases where PROJECT_NAME is defined, we simple assign this variable to MODULE_NAME.
# Be careful to correctly set MODULE_NAME, as it is also used in the uninstall.cmake.in files for naming the uninstall manifest.
# Additionally, install targets are linked to the MODULE_NAME and COMPONENT_NAME.

set(PLUGIN_NAME Commander)
set(MODULE_NAME WPEFramework${PLUGIN_NAME})

include(setup_target_properties_library)
include(show_target_properties)
include(display_list)

message("Setting up ${MODULE_NAME}")

find_package(WPEFramework QUIET)

if(CLION_ENVIRONMENT)
    set(WPEFRAMEWORK_LIBRARY_WPEFrameworkPlugins WPEFrameworkPlugins)
    set(WPEFRAMEWORK_LIBRARY_WPEFrameworkCore WPEFrameworkCore)
    set(WPEFRAMEWORK_LIBRARY_WPEFrameworkTracing WPEFrameworkTracing)
    set(WPEFRAMEWORK_INCLUDE_DIRS ${WPEFRAMEWORK_ROOT}/Source)
endif()

set(PLUGIN_DEFINITIONS
    )

set(PLUGIN_INCLUDE_DIRS
    ${WPEFRAMEWORK_INCLUDE_DIRS})

set(PLUGIN_OPTIONS)

set(PLUGIN_LINK_OPTIONS)

set(PLUGIN_DEPENDENCIES
    )

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
    Commander.cpp
    Commands.cpp
)

set(PLUGIN_INPUT
    ${PLUGIN_SOURCES}
    ${PLUGIN_INCLUDES}
    )

if (WPEFRAMEWORK_VERBOSE_BUILD)
    display_list("Defines                     : " ${PLUGIN_DEFINITIONS} )
    display_list("Compiler options            : " ${PLUGIN_OPTIONS} )
    display_list("Source files                : " ${PLUGIN_SOURCES} )
    display_list("Include files               : " ${PLUGIN_INCLUDES} )
    display_list("Include dirs                : " ${PLUGIN_INCLUDE_DIRS} )
    display_list("Link libs                   : " ${PLUGIN_LIBS} )
    display_list("Linker options              : " ${PLUGIN_LINK_OPTIONS} )
    display_list("Dependencies                : " ${PLUGIN_DEPENDENCIES} )
endif()

add_library(${MODULE_NAME} SHARED ${PLUGIN_INPUT})
target_compile_definitions(${MODULE_NAME} PRIVATE ${PLUGIN_DEFINITIONS})
target_include_directories(${MODULE_NAME} PRIVATE ${PLUGIN_INCLUDE_DIRS})
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
#    kv(callsign ${PLUGIN_NAME})
    kv(locator lib${MODULE_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
    kv(classname ${PLUGIN_NAME})
end()
ans(PLUGIN_CONFIG)

json_write("${CMAKE_CURRENT_LIST_DIR}/${PLUGIN_NAME}.json" ${PLUGIN_CONFIG})
install(
    FILES ${PLUGIN_NAME}.json DESTINATION
    ${CMAKE_INSTALL_PREFIX}/../etc/WPEFramework/plugins/
    COMPONENT ${MODULE_NAME})
