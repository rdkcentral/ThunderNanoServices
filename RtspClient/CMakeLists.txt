set(PLUGIN_NAME RtspClient)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(PLUGIN_RTSPCLIENT_AUTOSTART true CACHE STRING true)
set(PLUGIN_RTSPCLIENT_OOP true CACHE STRING true)

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED 
        Module.cpp
        RtspClient.cpp
        RtspClientImplementation.cpp
        RtspParser.cpp
        RtspParser.cpp
        RtspSession.cpp
        RtspSessionInfo.cpp
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
