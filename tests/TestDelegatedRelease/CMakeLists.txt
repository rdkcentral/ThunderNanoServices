set(PLUGIN_NAME TestDelegatedRelease)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

message("Setting up ${PLUGIN_NAME}")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

set(PLUGIN_TESTDELEGATEDRELEASE_STARTMODE "Deactivated" CACHE STRING "Startup mode of the TestDelegatedRelease plugin")
set(PLUGIN_TESTDELEGATEDRELEASE_RESUMED false CACHE BOOL "Start TestDelegatedRelease in resumed state")
set(PLUGIN_TESTDELEGATEDRELEASE_MODE "Local" CACHE STRING "Controls whether TestDelegatedRelease runs in its own process")

add_library(${MODULE_NAME} SHARED
    Module.cpp
    TestDelegatedRelease.cpp
)

set_target_properties(${MODULE_NAME} PROPERTIES
    CXX_STANDARD ${CXX_STD}
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins COMPONENT ${NAMESPACE}_Test)

write_config(PLUGINS ${PLUGIN_NAME} CLASSNAME ${PLUGIN_NAME})

option(TESTDELEGATEDRELEASE_TESTS "Build GTest suite for TestDelegatedRelease" OFF)
if(TESTDELEGATEDRELEASE_TESTS)
    add_subdirectory(tests)
endif()