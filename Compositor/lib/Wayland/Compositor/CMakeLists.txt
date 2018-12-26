set(TARGET ${PLATFORM_COMPOSITOR})

message("Setup ${TARGET} v${VERSION}...")

# add the platformplugin library
add_library(${TARGET} Wayland.cpp)

find_package(WaylandServer REQUIRED)
find_package(WaylandEGL REQUIRED)
find_package(Core REQUIRED)
find_package(Plugins REQUIRED)
find_package(EGL REQUIRED)

if (PLUGIN_COMPOSITOR_NXSERVER)
    add_definitions(-DENABLE_NXSERVER=1)
    find_package(Nexus REQUIRED)
    find_package(NXServer REQUIRED)
    target_link_libraries(${TARGET} PRIVATE nexus::nexusserver)
endif ()

target_link_libraries(${TARGET}
        PRIVATE
        ${CORE_LIBRARIES}
        ${PLUGINS_LIBRARIES}
        WAYLAND::SERVER
        WAYLAND::EGL
        EGL::EGL
        WPEFramework::WaylandEGLClient
        )

set_target_properties(${TARGET}
        PROPERTIES
            DEFINE_SYMBOL WPEFRAMEWORK_PLATFORM_PLUGIN
        )

find_package(Westeros)
if(WESTEROS_FOUND)
    target_sources(${TARGET} PRIVATE Westeros.cpp)
    target_link_libraries(${TARGET} PRIVATE WESTEROS::SERVER)
endif(WESTEROS_FOUND)

if(PLUGIN_COMPOSITOR_VIRTUALINPUT)
    find_package(VirtualInput REQUIRED)
    target_link_libraries(${TARGET} PRIVATE ${VIRTUALINPUT_LIBRARIES})
endif(PLUGIN_COMPOSITOR_VIRTUALINPUT)

install(
    TARGETS ${TARGET}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/Compositor
)

