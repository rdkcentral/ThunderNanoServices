
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(${NAMESPACE}Tracing CONFIG REQUIRED)
find_package(${NAMESPACE}LocalTracer CONFIG REQUIRED)

find_package(EGL REQUIRED)
find_package(GLESv2 REQUIRED)
find_package(libdrm REQUIRED)
find_package(gbm REQUIRED)

add_executable(glextinfo main.cpp)

target_link_libraries(glextinfo
    PUBLIC
        common::include
        gl::shaders
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Tracing::${NAMESPACE}Tracing
        ${NAMESPACE}LocalTracer::${NAMESPACE}LocalTracer
        libdrm::libdrm
        gbm::gbm
        EGL::EGL
        GLESv2::GLESv2
)

