set(PLUGIN_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkPlugins}
    ${PLUGIN_DEPENDENCIES})

set(PLUGIN_INCLUDE_DIRS
    ${WPEFRAMEWORK_INCLUDE_DIRS}
    ${PLUGIN_DEPENDENCIES_INCLUDE_DIRS})

file(GLOB PLUGIN_INCLUDES *.h)

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

include(setup_target_properties_library)
setup_target_properties_library(${MODULE_NAME})

if (WPEFRAMEWORK_VERBOSE_BUILD)
    include(show_target_properties)
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

macro(write_config)
    foreach(plugin ${ARGV})
        message("Writing configuration for ${plugin}")

        map()
            kv(locator lib${MODULE_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
            kv(classname ${PLUGIN_NAME})
        end()
        ans(plugin_config) # default configuration

        if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${plugin}.config")
            include(${CMAKE_CURRENT_LIST_DIR}/${plugin}.config)

            list(LENGTH preconditions number_preconditions)

            if (number_preconditions GREATER 0)
                map_append(${plugin_config} precondition ___array___)
                foreach(entry ${preconditions})
                    map_append(${plugin_config} precondition ${entry})
                endforeach()
            endif()

            if (NOT ${autostart} STREQUAL "")
                map_append(${plugin_config} autostart ${autostart})
            endif()

            if (NOT ${configuration} STREQUAL "")
                map_append(${plugin_config} configuration ${configuration})
            endif()
        endif()

        json_write("${CMAKE_CURRENT_LIST_DIR}/${plugin}.json" ${plugin_config})

        install(
                FILES ${plugin}.json DESTINATION
                ${CMAKE_INSTALL_PREFIX}/../etc/WPEFramework/plugins/
                COMPONENT ${MODULE_NAME})
    endforeach()
endmacro()


