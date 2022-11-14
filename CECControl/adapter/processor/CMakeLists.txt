add_library(_cec_message_processor STATIC
    Module.cpp
    CECMessage.cpp
    CECProcessor.cpp)

target_include_directories(_cec_message_processor
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(_cec_message_processor
    PUBLIC
        _cec_common_includes
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}COM::${NAMESPACE}COM
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        )

add_subdirectory(messages)

if(CEC_TESTS)
    add_subdirectory(test)
endif()

