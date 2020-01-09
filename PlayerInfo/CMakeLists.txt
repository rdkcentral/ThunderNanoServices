set(PLUGIN_NAME PlayerInfo)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
    PlayerInfo.cpp
    PlayerInfoJsonRpc.cpp
    PlatformImplementation.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

find_package(GSTREAMER REQUIRED)
find_package(GSTREAMER_BASE REQUIRED)

if (GSTREAMER_FOUND)
    target_sources(${MODULE_NAME}
        PRIVATE
            GStreamer/PlatformImplementation.cpp)

    target_link_libraries(${MODULE_NAME}
        PRIVATE
            ${GSTREAMER_LIBRARIES}
            ${GSTREAMER_BASE_LIBRARIES})

    target_include_directories(${MODULE_NAME}
        PRIVATE
            ${GSTREAMER_INCLUDES}
            ${GSTREAMER_BASE_INCLUDES})
endif ()

install(TARGETS ${MODULE_NAME} 
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
