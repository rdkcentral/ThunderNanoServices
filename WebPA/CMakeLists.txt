set(PLUGIN_NAME WebPA)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(WDMP-C REQUIRED)
find_package(ParodusService REQUIRED)


add_library(${MODULE_NAME} SHARED
    WebPA.cpp
    Module.cpp
    WebPAImplementation.cpp
)

target_compile_definitions(${MODULE_NAME} PRIVATE ${PLUGIN_DEFINITIONS})
target_include_directories(${MODULE_NAME} PRIVATE ${PLUGIN_INCLUDE_DIRS})

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ParodusService::ParodusService
        wdmp_c::wdmp_c
)

string(TOLOWER ${NAMESPACE} STORAGE_DIRECTORY)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

set(PLUGIN_WEBPA_AUTOSTART false CACHE STRING "Automatically start WebPA plugin")

write_config(${PLUGIN_NAME})

add_subdirectory(Clients)
