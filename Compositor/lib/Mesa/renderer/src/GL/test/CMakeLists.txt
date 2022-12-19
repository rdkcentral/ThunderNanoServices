
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(${NAMESPACE}Tracing CONFIG REQUIRED)
find_package(${NAMESPACE}LocalTracer CONFIG REQUIRED)

add_executable(glextinfo main.cpp)

target_link_libraries(glextinfo
    PUBLIC
        common::include
        gl::shaders
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Tracing::${NAMESPACE}Tracing
        ${NAMESPACE}LocalTracer::${NAMESPACE}LocalTracer
        ${COMPOSITOR_RENDERER_IMPLEMENTATION}
)

add_executable(testapp testapp.cpp)

target_link_libraries(testapp
    PUBLIC
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Tracing::${NAMESPACE}Tracing
        ${NAMESPACE}LocalTracer::${NAMESPACE}LocalTracer
)

