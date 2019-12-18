cmake_minimum_required(VERSION 3.3)

project(Cobalt)

find_package(WPEFramework)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(PLUGIN_NAME Cobalt)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(Cobalt REQUIRED)

get_filename_component(CONFIG_HEADER_NAME ${WPEFRAMEWORK_PLUGIN_COBALT_STARBOARD_CONFIGURATION_INCLUDE} NAME)
get_filename_component(CONFIG_HEADER_DIRECTORY ${WPEFRAMEWORK_PLUGIN_COBALT_STARBOARD_CONFIGURATION_INCLUDE} DIRECTORY)

find_file(CONFIG_HEADER  ${CONFIG_HEADER_NAME}
    PATH_SUFFIXES ${CONFIG_HEADER_DIRECTORY})

if(NOT CONFIG_HEADER)
    message(FATAL_ERROR "Cobalt configurtion not found: '${CONFIG_HEADER_DIRECTORY}/${CONFIG_HEADER_NAME}'")    
endif()

add_library(${MODULE_NAME} SHARED 
    Cobalt.cpp
    CobaltImplementation.cpp
    CobaltJsonRpc.cpp
    Module.cpp)

target_compile_definitions(${MODULE_NAME} 
    PRIVATE 
        STARBOARD 
        STARBOARD_CONFIGURATION_INCLUDE="${WPEFRAMEWORK_PLUGIN_COBALT_STARBOARD_CONFIGURATION_INCLUDE}")

target_link_libraries(${MODULE_NAME} 
    PRIVATE 
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${COBALT_LIBRARIES}
        )

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

set(PLUGIN_COBALT_AUTOSTART false CACHE STRING "Automatically start Cobalt plugin")
set(PLUGIN_COBALT_OUTOFPROCESS false CACHE STRING "Controls if the plugin should run in its own process")

write_config(${PLUGIN_NAME})
