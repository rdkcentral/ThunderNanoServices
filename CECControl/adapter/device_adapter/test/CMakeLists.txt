find_package(Thunder)

find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Messaging REQUIRED)
find_package(${NAMESPACE}LocalTracer REQUIRED)

add_executable(cec_device_adapter_test
    cec_device_adapter_test.cpp
)

target_include_directories(cec_device_adapter_test
PRIVATE
    ${CEC_INCLUDE_DIR})

set_target_properties(cec_device_adapter_test PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        ENABLE_EXPORTS FALSE)

target_link_libraries(cec_device_adapter_test PRIVATE
    _cec_common_includes
    ${CEC_DEVICE_ADAPTER_LIBRARY_NAME}
    CompileSettingsDebug::CompileSettingsDebug
    ${NAMESPACE}Core::${NAMESPACE}Core
    ${NAMESPACE}LocalTracer::${NAMESPACE}LocalTracer
    ${NAMESPACE}Messaging::${NAMESPACE}Messaging)

install(TARGETS cec_device_adapter_test DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Test)
