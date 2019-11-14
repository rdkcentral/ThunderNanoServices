set(PLUGIN_NAME FileTransfer)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)

add_library(${MODULE_NAME} SHARED
    FileTransfer.cpp
    Module.cpp)

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
