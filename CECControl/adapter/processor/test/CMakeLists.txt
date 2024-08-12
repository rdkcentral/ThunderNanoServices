find_package(Thunder)

find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Messaging REQUIRED)
find_package(${NAMESPACE}LocalTracer REQUIRED)

add_executable(cec_processor_test
    cec_processor_test.cpp
)

set_target_properties(cec_processor_test PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        ENABLE_EXPORTS FALSE)

target_link_libraries(cec_processor_test
PRIVATE
    _cec_common_includes
    ${CEC_DEVICE_ADAPTER_LIBRARY_NAME}
    _cec_message_processor
    CompileSettingsDebug::CompileSettingsDebug
    ${NAMESPACE}Core::${NAMESPACE}Core
    ${NAMESPACE}LocalTracer::${NAMESPACE}LocalTracer)

install(TARGETS cec_processor_test DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Test)

#set(CMAKE_DEBUG_TARGET_PROPERTIES COMPILE_OPTIONS COMPILE_FEATURES)
