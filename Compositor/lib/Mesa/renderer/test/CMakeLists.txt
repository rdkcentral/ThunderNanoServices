
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(${NAMESPACE}Messaging CONFIG REQUIRED)
find_package(${NAMESPACE}LocalTracer CONFIG REQUIRED)

add_subdirectory(simpleworker)
add_subdirectory(texturebuffer)

add_executable(testclear testclear.cpp)

target_link_libraries(testclear
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
        CompositorSimpleWorker::CompositorSimpleWorker
)

install(TARGETS testclear DESTINATION bin/${NAMESPACE}Tests)


add_executable(testquads testquads.cpp)

target_link_libraries(testquads
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
        CompositorSimpleWorker::CompositorSimpleWorker
)

install(TARGETS testquads DESTINATION bin/${NAMESPACE}Tests)


add_executable(testtexture testtexture.cpp)

target_link_libraries(testtexture
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
        CompositorSimpleWorker::CompositorSimpleWorker
        TextureBuffer::TextureBuffer
)

install(TARGETS testtexture DESTINATION bin/${NAMESPACE}Tests)

add_executable(testdmabuf testdmabuf.cpp)

target_link_libraries(testdmabuf
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
        CompositorSimpleWorker::CompositorSimpleWorker
)

install(TARGETS testdmabuf DESTINATION bin/${NAMESPACE}Tests)