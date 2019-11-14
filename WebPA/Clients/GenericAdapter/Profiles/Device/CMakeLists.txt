set(TARGET Device)

find_package(WPEFramework REQUIRED)
find_package(jsonrpc REQUIRED)
find_package(Procps REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

file(GLOB IFACE_CONTROL_PLUGIN_INCLUDES *.h)

add_library(${TARGET}
    Module.cpp
    DeviceControl.cpp
    DeviceInfo.cpp
)

target_include_directories(${TARGET}
    PRIVATE
        ../
        ../../
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        jsonrpc::jsonrpc
        procps::procps
)

set_target_properties(${TARGET} PROPERTIES SUFFIX ".profile")
set_target_properties(${TARGET} PROPERTIES PREFIX "")

install(TARGETS ${TARGET} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/WPEFramework/${PLUGIN_NAME})
