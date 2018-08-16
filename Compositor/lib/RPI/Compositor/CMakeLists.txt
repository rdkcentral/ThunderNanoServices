set(TARGET RPICompositor)

message("Setting up ${TARGET} for RPI platfrom")

find_package(Core REQUIRED)

add_library(${TARGET}
        RPI.cpp
        )

target_link_libraries(${TARGET}
        PRIVATE
        WPEFramework::Core
        WPEFramework::Plugins
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
