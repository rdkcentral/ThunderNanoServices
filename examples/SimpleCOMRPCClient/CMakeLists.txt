find_package(${NAMESPACE}Protocols REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_executable(SimpleCOMRPCClient SimpleCOMRPCClient.cpp)

set_target_properties(SimpleCOMRPCClient PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )     

target_link_libraries(SimpleCOMRPCClient 
        PRIVATE
        ${NAMESPACE}Protocols::${NAMESPACE}Protocols
        CompileSettingsDebug::CompileSettingsDebug
    )

install(TARGETS SimpleCOMRPCClient DESTINATION bin)
