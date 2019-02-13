set(TARGET ${PLATFORM_COMPOSITOR})

message("Setup ${TARGET} v${VERSION}...")

# add the platformplugin library
add_library(${TARGET} Wayland.cpp)

find_package(WaylandServer REQUIRED)
find_package(WaylandEGL REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Interfaces REQUIRED)
find_package(EGL REQUIRED)

if (PLUGIN_COMPOSITOR_NXSERVER)
    find_package(NEXUS REQUIRED)
    find_package(NXSERVER REQUIRED)
    target_link_libraries(${TARGET} 
        PRIVATE 
           ${NAMESPACE}nexusserver::${NAMESPACE}nexusserver)
    target_compile_definitions(${TARGET}
        PRIVATE
            ENABLE_NXSERVER=1)
endif ()

target_link_libraries(${TARGET}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Interfaces::${NAMESPACE}Interfaces
        WaylandServer::WaylandServer
        WaylandEGL::WaylandEGL
        ${EGL_LIBRARIES}
)

target_include_directories(${TARGET}
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../Client>
        ${EGL_INCLUDE_DIRS}
)

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        DEFINE_SYMBOL WPEFRAMEWORK_PLATFORM_PLUGIN
)

find_package(WesterosServer)
if(WESTEROS_SERVER_FOUND)
    target_sources(${TARGET} 
        PRIVATE 
            Westeros.cpp
    )
    target_link_libraries(${TARGET} 
        PRIVATE
            WesterosServer::WesterosServer
    )
endif(WESTEROS_SERVER_FOUND)

if(PLUGIN_COMPOSITOR_VIRTUALINPUT)
    find_package(${NAMESPACE}VirtualInput REQUIRED)
    target_link_libraries(${TARGET} 
        PRIVATE 
           ${NAMESPACE}VirtualInput::${NAMESPACE}VirtualInput
    )
endif(PLUGIN_COMPOSITOR_VIRTUALINPUT)

install(
    TARGETS ${TARGET}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/Compositor
)

