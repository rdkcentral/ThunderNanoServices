set(PLUGIN_NAME LocationSync)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(PLUGIN_LOCATIONSYNC_URI location.webplatformforembedded.org CACHE STRING "URI to request the location information")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED 
    Module.cpp
    LocationSync.cpp
    LocationService.cpp
    LocationSyncJsonRpc.cpp
)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME} 
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

install(TARGETS ${MODULE_NAME} 
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
