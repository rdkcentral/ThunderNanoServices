project(TestPerCallsignRegistration)

cmake_minimum_required(VERSION 3.15)

find_package(Thunder)
project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})
message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
    Module.cpp
    TestPerCallsignRegistration.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
    CXX_STANDARD ${CXX_STD}
    CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGE_DIRECTORY}/plugins
    COMPONENT ${NAMESPACE}_Test)

set(TESTPERCALLSIGNREGISTRATION_STARTMODE "Deactivated"
    CACHE STRING "Startup mode of the per-callsign registration test plugin")
set(TESTPERCALLSIGNREGISTRATION_RESUMED false
    CACHE BOOL "Start the per-callsign registration test plugin resumed")

write_config()

option(TESTPERCALLSIGNREGISTRATION_TESTS
    "Build in-process tests for per-callsign IShell and IController registration"
    OFF)

if(TESTPERCALLSIGNREGISTRATION_TESTS)
    add_subdirectory(tests)
endif()