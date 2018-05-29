# WPEFRAMEWORK_ROOT acts as the root directory for this project, in case it is embedded in a larger, surrounding project.
# COMPONENT_NAME is used to group multiple install targets (one per module).
# MODULE_NAME is used as a replacement for PROJECT_NAME, as using project(x) and PROJECT_NAME seems to interfere with
# Yocto builds. In some cases where PROJECT_NAME is defined, we simple assign this variable to MODULE_NAME.
# Be careful to correctly set MODULE_NAME, as it is also used in the uninstall.cmake.in files for naming the uninstall manifest.
# Additionally, install targets are linked to the MODULE_NAME and COMPONENT_NAME.

set(PLUGIN_NAME TVControl)
set(MODULE_NAME WPEFramework${PLUGIN_NAME})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
set(WPEFRAMEWORK_PLUGIN_TVCONTROL_DVB false CACHE STRING "Set dvb dbs")

include(setup_target_properties_library)
include(show_target_properties)

message("Setting up ${MODULE_NAME}")

find_package(WPEFramework QUIET)
find_package(DVBApps)
find_package(LibSqlite3 REQUIRED)

if (CLION_ENVIRONMENT)
    set(WPEFRAMEWORK_LIBRARY_WPEFrameworkPlugins WPEFrameworkPlugins)
    set(WPEFRAMEWORK_INCLUDE_DIRS ${WPEFRAMEWORK_ROOT}/Source)
endif ()

set(PLUGIN_TVCONTROL_DEFINITIONS
    PLUGIN_TVCONTROL
    TV_DEBUG
    )

set(PLUGIN_TVCONTROL_INCLUDE_DIRS
    ${WPEFRAMEWORK_INCLUDE_DIRS})

set(PLUGIN_TVCONTROL_OPTIONS
    )

set(PLUGIN_TVCONTROL_LINK_OPTIONS
    )

set(PLUGIN_TVCONTROL_DEPENDENCIES
    )

set(PLUGIN_TVCONTROL_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkPlugins}
    ${PLUGIN_TVCONTROL_DEPENDENCIES})

file(GLOB PLUGIN_TVCONTROL_INCLUDES *.h
    )

set(PLUGIN_TVCONTROL_SOURCES
    Module.cpp
    TVControl.cpp
    TVControlImplementation.cpp

    TableData/ITableData.cpp

    TableData/Data/EPGData.cpp

    Tuner/ITuner.cpp
    )

list(APPEND PLUGIN_TVCONTROL_INCLUDE_DIRS . TVControl Tuner TableData TableData/Data TableData/Parser)

if (WPEFRAMEWORK_PLUGIN_TVCONTROL_DVB)
    list(APPEND PLUGIN_TVCONTROL_SOURCES TableData/Parser/DVB/ParserDVB.cpp)
    list(APPEND PLUGIN_TVCONTROL_INCLUDE_DIRS TableData/Parser/DVB)
    if(WPEFRAMEWORK_PLUGIN_TVCONTROL_ENABLE_BOUQUET_PARSING)
        add_definitions(-DENABLE_BOUQUET_PARSING=1)
    endif()
else ()
    if (DVBAPPS_FOUND)
        list(APPEND PLUGIN_TVCONTROL_LIBS ${DVBAPPS_LIBRARIES})
    endif ()
    list(APPEND PLUGIN_TVCONTROL_SOURCES TableData/Parser/ATSC/ParserATSC.cpp)
    list(APPEND PLUGIN_TVCONTROL_INCLUDE_DIRS TableData/Parser/ATSC)
endif ()
if (LIBSQLITE3_FOUND)
    list(APPEND PLUGIN_TVCONTROL_INCLUDE_DIRS ${LIBSQLITE3_INCLUDE_DIRS})
    list(APPEND PLUGIN_TVCONTROL_LIBS ${LIBSQLITE3_LIBRARIES})
endif ()

set(PLUGIN_TVCONTROL_INPUT
    ${PLUGIN_TVCONTROL_SOURCES}
    ${PLUGIN_TVCONTROL_INCLUDES}
    )



display_list("Defines                     : " ${PLUGIN_TVCONTROL_DEFINITIONS})
display_list("Compiler options            : " ${PLUGIN_TVCONTROL_OPTIONS})
display_list("Source files                : " ${PLUGIN_TVCONTROL_SOURCES})
display_list("Include files               : " ${PLUGIN_TVCONTROL_INCLUDES})
display_list("Include dirs                : " ${PLUGIN_TVCONTROL_INCLUDE_DIRS})
display_list("Link libs                   : " ${PLUGIN_TVCONTROL_LIBS})
display_list("Linker options              : " ${PLUGIN_TVCONTROL_LINK_OPTIONS})
display_list("Dependencies                : " ${PLUGIN_TVCONTROL_DEPENDENCIES})

add_library(${MODULE_NAME} SHARED ${PLUGIN_TVCONTROL_INPUT})
target_compile_definitions(${MODULE_NAME} PRIVATE ${PLUGIN_TVCONTROL_DEFINITIONS})
target_include_directories(${MODULE_NAME} PRIVATE ${PLUGIN_TVCONTROL_INCLUDE_DIRS})
target_link_libraries(${MODULE_NAME} ${PLUGIN_TVCONTROL_LIBS})
list_to_string(PLUGIN_TVCONTROL_LINK_OPTIONS PLUGIN_TVCONTROL_LINK_OPTIONS_STRING)
if (NOT "${PLUGIN_TVCONTROL_LINK_OPTIONS_STRING}" STREQUAL "")
    set_target_properties(${MODULE_NAME} PROPERTIES LINK_FLAGS "${PLUGIN_TVCONTROL_LINK_OPTIONS_STRING}")
endif ()
set_target_properties(${MODULE_NAME} PROPERTIES OUTPUT_NAME ${MODULE_NAME})

setup_target_properties_library(${MODULE_NAME})

show_target_properties(${MODULE_NAME})

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

# default configutation
map()
#  kv(callsign Tuner)
    kv(locator lib${MODULE_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
    kv(classname ${PLUGIN_NAME})
    kv(autostart true)
    kv(outofprocess true)
    key(configuration)
    map()
        kv(dvb ${WPEFRAMEWORK_PLUGIN_TVCONTROL_DVB})
        kv(idletime 30)
        kv(homets 0)
        kv(frequencylist ${WPEFRAMEWORK_PLUGINS_TVCONTROL_FREQUENCY_LIST})
        kv(tuneparameters ${WPEFRAMEWORK_PLUGINS_TVCONTROL_TUNE_PARAM})
        kv(countrycode ${WPEFRAMEWORK_PLUGINS_TVCONTROL_COUNTRY_CODE})
        kv(countryregionid ${WPEFRAMEWORK_PLUGINS_TVCONTROL_COUNTRY_REGION_ID})
    end()
end()
ans(PLUGIN_TVCONTROL_CONFIG)

json_write("${CMAKE_CURRENT_LIST_DIR}/${PLUGIN_NAME}.json" ${PLUGIN_TVCONTROL_CONFIG})
install(
    FILES ${PLUGIN_NAME}.json
    DESTINATION ${CMAKE_INSTALL_PREFIX}/../etc/WPEFramework/plugins/
    COMPONENT ${MODULE_NAME})

add_subdirectory(TVPlatform)
add_subdirectory(test)

