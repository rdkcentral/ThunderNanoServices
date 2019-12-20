set(PLUGIN_NAME OutOfProcessPlugin)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

 message("Setting up ${PLUGIN_NAME}")

 find_package(${NAMESPACE}Plugins REQUIRED)
 find_package(${NAMESPACE}Definitions REQUIRED)
 find_package(CompileSettingsDebug CONFIG REQUIRED)

 add_library(${MODULE_NAME} SHARED
        Module.cpp
        OutOfProcessPlugin.cpp
        OutOfProcessImplementation.cpp
)

 set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

 target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

 install(TARGETS ${MODULE_NAME}
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

 write_config(${PLUGIN_NAME})
