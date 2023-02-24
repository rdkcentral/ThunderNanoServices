
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(${NAMESPACE}Tracing CONFIG REQUIRED)
find_package(${NAMESPACE}LocalTracer CONFIG REQUIRED)
find_package(EGL REQUIRED)

add_executable(glextinfo glextinfo.cpp)

target_link_libraries(glextinfo
    PUBLIC
        common::include
        gl::shaders
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Tracing::${NAMESPACE}Tracing
        ${NAMESPACE}LocalTracer::${NAMESPACE}LocalTracer
        ${COMPOSITOR_RENDERER_IMPLEMENTATION}
        common::renderer
        EGL::EGL
)

install(TARGETS glextinfo DESTINATION bin)

add_executable(renderertest renderertest.cpp)

target_link_libraries(renderertest
    PUBLIC
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Tracing::${NAMESPACE}Tracing
        ${NAMESPACE}LocalTracer::${NAMESPACE}LocalTracer
        gl::shaders
        common::include
        common::buffer
        common::backend
        common::renderer
)

install(TARGETS renderertest DESTINATION bin)
