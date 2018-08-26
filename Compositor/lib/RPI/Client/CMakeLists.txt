set(TARGET compositorclient)

message("Setting up ${TARGET} for RPI platfrom")

find_package(Core REQUIRED)
find_package(LibVirtualKeyboard REQUIRED)
find_package(VirtualInput REQUIRED)
find_package(Plugins REQUIRED)

add_library(${TARGET}
        Implementation.cpp
        )

target_link_libraries(${TARGET}
        PUBLIC
        WPEFramework::Core
        WPEFramework::Plugins
        WPEFramework::VirtualInput
        )

set_target_properties(${TARGET}
        PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        OUTPUT_NAME ${compositor_artefact}
        )

install(TARGETS ${TARGET}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/WPEFramework/Compositor
        )
