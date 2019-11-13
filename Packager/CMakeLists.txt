set(PLUGIN_NAME Packager)
set(MODULE_NAME WPEFramework${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(libprovision REQUIRED)
find_package(LibOPKG REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
    Module.cpp
    Packager.cpp
    PackagerImplementation.cpp)

target_link_libraries(${MODULE_NAME} 
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        libprovision::libprovision
        LibOPKG::LibOPKG
        )

string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} 
    DESTINATION lib/${STORAGENAME}/plugins)

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/opkg.conf.in"
        "${CMAKE_CURRENT_BINARY_DIR}/opkg.conf"
        @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/opkg.conf
        DESTINATION "/usr/share/${NAMESPACE}/${PLUGIN_NAME}")

write_config(${PLUGIN_NAME})
