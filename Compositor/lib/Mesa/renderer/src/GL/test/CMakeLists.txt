
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(${NAMESPACE}Messaging CONFIG REQUIRED)
find_package(${NAMESPACE}LocalTracer CONFIG REQUIRED)
find_package(EGL REQUIRED)

add_executable(glextinfo glextinfo.cpp)

target_link_libraries(glextinfo
    PRIVATE
        common::include
        gl::shaders
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        ${NAMESPACE}LocalTracer::${NAMESPACE}LocalTracer
        common::renderer
        EGL::EGL
)

install(TARGETS glextinfo DESTINATION ${CMAKE_INSTALL_BINDIR}/${NAMESPACE}Tests COMPONENT ${NAMESPACE}_Test)
