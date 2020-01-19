find_package(${NAMESPACE}Protocols REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_executable(COMRPCClient COMRPCClient.cpp)

set_target_properties(COMRPCClient PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )     

target_link_libraries(COMRPCClient 
        PRIVATE
        ${NAMESPACE}Protocols::${NAMESPACE}Protocols
        CompileSettingsDebug::CompileSettingsDebug
    )

install(TARGETS COMRPCClient DESTINATION bin)
