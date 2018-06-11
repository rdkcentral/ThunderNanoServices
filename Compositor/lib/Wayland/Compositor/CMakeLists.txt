set(target_name ${compositor_artefact})

message("Setup ${target_name} v${VERSION}...")

# add the platformplugin library
add_library(${target_name} Wayland.cpp)

find_package(WaylandServer REQUIRED)
find_package(WaylandEGL REQUIRED)
find_package(Core REQUIRED)
find_package(Plugins REQUIRED)
find_package(EGL REQUIRED)

#find_package(nexusserver)

if (WPEFRAMEWORK_PLUGIN_COMPOSITOR_NXSERVER)
    find_package(Nexus REQUIRED)
    find_package(NXServer REQUIRED)
    target_link_libraries(${target_name} PRIVATE nexus::nexusserver)
endif ()

target_link_libraries(${target_name}
        PRIVATE
        WPEFramework::Core
        WPEFramework::Plugins
        WAYLAND::SERVER
        WAYLAND::EGL
        EGL::EGL
        WPEFramework::WaylandEGLClient
        )

set_target_properties(${target_name}
        PROPERTIES
            DEFINE_SYMBOL WPEFRAMEWORK_PLATFORM_PLUGIN
        )

find_package(Westeros)
if(WESTEROS_FOUND)
    target_sources(${target_name} PRIVATE Westeros.cpp)
    target_link_libraries(${target_name} PRIVATE WESTEROS::SERVER)
endif(WESTEROS_FOUND)

if(WPEFRAMEWORK_PLUGIN_COMPOSITOR_VIRTUALINPUT)
    find_package(VirtualInput REQUIRED)
    target_link_libraries(${target_name} PRIVATE WPEFramework::VirtualInput)
endif(WPEFRAMEWORK_PLUGIN_COMPOSITOR_VIRTUALINPUT)

install(
    TARGETS ${target_name}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/WPEFramework/Compositor
)

