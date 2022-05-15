add_library(_cec_common_includes INTERFACE)

target_link_libraries(_cec_common_includes
    INTERFACE
        ${NAMESPACE}Core::${NAMESPACE}Core)

target_include_directories(_cec_common_includes
    INTERFACE
        ${PROJECT_SOURCE_DIR}/adapter/include
        ${PROJECT_SOURCE_DIR}/adapter/device_adapter/include)
