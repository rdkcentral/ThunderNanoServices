find_package(${NAMESPACE}Protocols REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_executable(JSONRPCClient JSONRPCClient.cpp)

set_target_properties(JSONRPCClient PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )     

target_link_libraries(JSONRPCClient 
        PRIVATE
        ${NAMESPACE}Protocols::${NAMESPACE}Protocols
        CompileSettingsDebug::CompileSettingsDebug
    )

target_include_directories(JSONRPCClient 
    PRIVATE
         "${PROJECT_SOURCE_DIR}/tests"
)

install(TARGETS JSONRPCClient DESTINATION bin)
