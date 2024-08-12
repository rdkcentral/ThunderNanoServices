
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(${NAMESPACE}Messaging CONFIG REQUIRED)
find_package(${NAMESPACE}LocalTracer CONFIG REQUIRED)

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
)

set_target_properties(testclear PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
)

install(TARGETS testclear DESTINATION ${CMAKE_INSTALL_BINDIR}/${NAMESPACE}Tests COMPONENT ${NAMESPACE}_Test)

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
)

set_target_properties(testquads PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
)

install(TARGETS testquads DESTINATION ${CMAKE_INSTALL_BINDIR}/${NAMESPACE}Tests COMPONENT ${NAMESPACE}_Test)


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
        PixelBuffer::PixelBuffer
)

set_target_properties(testtexture PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
)

install(TARGETS testtexture DESTINATION ${CMAKE_INSTALL_BINDIR}/${NAMESPACE}Tests COMPONENT ${NAMESPACE}_Test)

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
        DmaBuffer::DmaBuffer
)

set_target_properties(testdmabuf PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
)

target_link_directories(testdmabuf PUBLIC ${CMAKE_CURRENT_LIST_DIR/../src/GL})

install(TARGETS testdmabuf DESTINATION ${CMAKE_INSTALL_BINDIR}/${NAMESPACE}Tests COMPONENT ${NAMESPACE}_Test)