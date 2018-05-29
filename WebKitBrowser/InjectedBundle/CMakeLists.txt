# WPEFRAMEWORK_ROOT acts as the root directory for this project, in case it is embedded in a larger, surrounding project.
# COMPONENT_NAME is used to group multiple install targets (one per module).
# MODULE_NAME is used as a replacement for PROJECT_NAME, as using project(x) and PROJECT_NAME seems to interfere with
# Yocto builds. In some cases where PROJECT_NAME is defined, we simple assign this variable to MODULE_NAME.
# Be careful to correctly set MODULE_NAME, as it is also used in the uninstall.cmake.in files for naming the uninstall manifest.
# Additionally, install targets are linked to the MODULE_NAME and COMPONENT_NAME.

set(MODULE_NAME WPEInjectedBundle)

include(setup_target_properties_library)
include(show_target_properties)

message("Setting up ${MODULE_NAME}")

find_package(WPEFramework QUIET)

set(WPEINJECTED_BUNDLE_DEFINITIONS
    WPEFRAMEWORK_WPEFRAMEWORKSUPPORT)

set(WPEINJECTED_BUNDLE_INCLUDE_DIRS
    ${WPEFRAMEWORK_INCLUDE_DIRS}
    ${WPE_WEBKIT_INCLUDE_DIRS}
    ${WPE_WEBKIT_INCLUDE_DIRS}/WPE)

set(WPEINJECTED_BUNDLE_OPTIONS)

set(WPEINJECTED_BUNDLE_LINK_OPTIONS)

set(WPEINJECTED_BUNDLE_DEPENDENCIES
    )

set(WPEINJECTED_BUNDLE_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    ${WPE_WEBKIT_LIBRARIES}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkProtocols}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkCore}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkTracing}
    ${WPEINJECTED_BUNDLE_DEPENDENCIES})

file(GLOB WPEINJECTED_BUNDLE_INCLUDES *.h)

set(WPEINJECTED_BUNDLE_SOURCES
    main.cpp
    WhiteListedOriginDomainsList.cpp
    Utils.cpp
    JavaScriptFunction.cpp
    NotifyWPEFramework.cpp
    Milestone.cpp
    ClassDefinition.cpp
)

set(WPEINJECTED_BUNDLE_INPUT
    ${WPEINJECTED_BUNDLE_SOURCES}
    ${WPEINJECTED_BUNDLE_INCLUDES}
    )

if (WPEFRAMEWORK_VERBOSE_BUILD)
    display_list("Defines                     : " ${WPEINJECTED_BUNDLE_DEFINITIONS} )
    display_list("Compiler options            : " ${WPEINJECTED_BUNDLE_OPTIONS} )
    display_list("Source files                : " ${WPEINJECTED_BUNDLE_SOURCES} )
    display_list("Include files               : " ${WPEINJECTED_BUNDLE_INCLUDES} )
    display_list("Include dirs                : " ${WPEINJECTED_BUNDLE_INCLUDE_DIRS} )
    display_list("Link libs                   : " ${WPEINJECTED_BUNDLE_LIBS} )
    display_list("Linker options              : " ${WPEINJECTED_BUNDLE_LINK_OPTIONS} )
    display_list("Dependencies                : " ${WPEINJECTED_BUNDLE_DEPENDENCIES} )
endif()

add_library(${MODULE_NAME} SHARED ${WPEINJECTED_BUNDLE_INPUT})
target_compile_definitions(${MODULE_NAME} PRIVATE ${WPEINJECTED_BUNDLE_DEFINITIONS})
target_include_directories(${MODULE_NAME} PRIVATE ${WPEINJECTED_BUNDLE_INCLUDE_DIRS})
target_link_libraries(${MODULE_NAME} ${WPEINJECTED_BUNDLE_LIBS})
list_to_string(WPEINJECTED_BUNDLE_LINK_OPTIONS WPEINJECTED_BUNDLE_LINK_OPTIONS_STRING)
if (NOT "${WPEINJECTED_BUNDLE_LINK_OPTIONS_STRING}" STREQUAL "")
    set_target_properties(${MODULE_NAME} PROPERTIES LINK_FLAGS "${WPEINJECTED_BUNDLE_LINK_OPTIONS_STRING}")
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
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/WPEFramework/WebKitBrowser
    COMPONENT ${MODULE_NAME})
