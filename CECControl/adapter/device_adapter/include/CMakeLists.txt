add_library(_cec_device_adapter_include INTERFACE)

target_include_directories(_cec_device_adapter_include
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR})

add_library(device_adapter_include::device_adapter_include _cec_device_adapter_include ALIAS)
