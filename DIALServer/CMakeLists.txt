cmake_minimum_required(VERSION 3.3)

project(DIALServer)

find_package(WPEFramework)

set(PLUGIN_NAME DIALServer)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

option(ENABLE_YOUTUBE "Enable YouTube support for DIAL server" OFF)
option(ENABLE_NETFLIX "Enable Netflix support for DIAL server" OFF)

find_package(${NAMESPACE}Plugins REQUIRED)

add_library(${MODULE_NAME} SHARED
    DIALServer.cpp
    Module.cpp)

if(ENABLE_YOUTUBE)
    target_sources(${MODULE_NAME}
        PRIVATE  
            YouTube.cpp)
endif()

if(ENABLE_NETFLIX)
    target_sources(${MODULE_NAME}
        PRIVATE  
            Netflix.cpp)
endif()

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config(${PLUGIN_NAME})
