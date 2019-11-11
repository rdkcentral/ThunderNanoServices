set(TARGET GenericAdapter)
message("Setup ${TARGET} v${VERSION}...")

find_package(WPEFramework)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(WDMP-C REQUIRED)
find_package(TinyXML REQUIRED)
find_package(LibParodus REQUIRED)
find_package(GLIB REQUIRED)


add_library(${TARGET}
    Handler/Handler.cpp
    Adapter/DataModel/DataModel.cpp
    Adapter/Notifier.cpp
    Adapter/Parameter.cpp
    Adapter/Attribute.cpp
    Adapter/Adapter.cpp
    Implementation.cpp
    Module.cpp
)

target_include_directories(${TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        Adapter
        Adapter/DataModel
        Handler
        ${GLIB_INCLUDE_DIRS}
)

target_link_libraries(${TARGET}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        wdmp_c::wdmp_c
        tinyxml::tinyxml
        libparodus::libparodus
        ${GLIB_LIBRARIES}
)

target_compile_definitions(${TARGET} PRIVATE ${PLUGIN_DEFINITIONS})
target_include_directories(${TARGET} PRIVATE ${PLUGIN_INCLUDE_DIRS})

set_target_properties(${TARGET} PROPERTIES SUFFIX ".webpa")
set_target_properties(${TARGET} PROPERTIES PREFIX "")
install(
    TARGETS ${TARGET}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/WebPA
)
install(
    FILES data-model.xml
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/WebPA)

add_subdirectory(Profiles)
