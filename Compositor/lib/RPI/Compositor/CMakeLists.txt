set(TARGET ${PLATFORM_COMPOSITOR})

message("Setting up ${TARGET} for RPI platfrom")

find_package(Core REQUIRED)
find_package(Plugins REQUIRED)
find_package(Interfaces REQUIRED)

add_library(${TARGET}
        RPI.cpp
        )

target_link_libraries(${TARGET}
        PRIVATE
        ${CORE_LIBRARIES}
        ${PLUGINS_LIBRARIES}
        ${INTERFACES_LIBRARIES}
        )

set_target_properties(${TARGET}
        PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        )

install(TARGETS ${TARGET}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/Compositor
        )
