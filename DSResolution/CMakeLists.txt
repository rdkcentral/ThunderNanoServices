set(PLUGIN_NAME DSResolution)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(PLUGIN_DSRESOLUTION_AUTOSTART false CACHE STRING "Automatically start DSResolution Plug-in")

find_package(DSHAL REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED 
    DSResolution.cpp
    DSResolutionHAL.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES)


target_link_libraries(${MODULE_NAME} 
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins 
        ${DSHAL_LIB})

install(TARGETS ${MODULE_NAME} 
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

if(DSRESOLUTION_WITH_DUMMY_DSHAL)
add_subdirectory(src)
endif(DSRESOLUTION_WITH_DUMMY_DSHAL)

write_config(${PLUGIN_NAME})
