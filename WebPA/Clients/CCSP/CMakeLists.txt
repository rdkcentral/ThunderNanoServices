set(TARGET CCSP)
message("Setup ${TARGET} v${VERSION}...")

find_package(WPEFramework)
find_package(CJSON REQUIRED)
find_package(WDMP-C REQUIRED)
find_package(LibParodus REQUIRED)
find_package(Parodus2CCSP REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${TARGET} CCSP.cpp)

target_link_libraries(${TARGET}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        parodus2ccsp::parodus2ccsp
)

target_compile_definitions(${TARGET} PRIVATE ${PLUGIN_DEFINITIONS})
target_include_directories(${TARGET} PRIVATE ${PLUGIN_INCLUDE_DIRS})

set_target_properties(${TARGET} PROPERTIES SUFFIX ".webpa")
set_target_properties(${TARGET} PROPERTIES PREFIX "")
install(
    TARGETS ${TARGET}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/WebPA
)

