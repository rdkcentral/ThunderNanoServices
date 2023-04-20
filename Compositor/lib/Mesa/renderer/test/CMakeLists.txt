
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(${NAMESPACE}Messaging CONFIG REQUIRED)
find_package(${NAMESPACE}LocalTracer CONFIG REQUIRED)

add_executable(renderertest renderertest.cpp)

target_link_libraries(renderertest
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        ${NAMESPACE}LocalTracer::${NAMESPACE}LocalTracer
        common::include
        common::buffer
        common::backend
        common::renderer
        common::drm
)

install(TARGETS renderertest DESTINATION bin)
