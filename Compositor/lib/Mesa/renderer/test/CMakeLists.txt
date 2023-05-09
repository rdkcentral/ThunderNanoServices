
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(${NAMESPACE}Messaging CONFIG REQUIRED)
find_package(${NAMESPACE}LocalTracer CONFIG REQUIRED)

add_subdirectory(simpleworker)

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
        SimpleWorker::SimpleWorker
)

install(TARGETS testclear DESTINATION bin)


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
        SimpleWorker::SimpleWorker
)

install(TARGETS testquads DESTINATION bin)