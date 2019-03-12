find_package(${NAMESPACE}JSONRPC REQUIRED)

add_executable(JSONRPCClient JSONRPCClient.cpp)

set_target_properties(JSONRPCClient PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )     

target_link_libraries(JSONRPCClient 
    PRIVATE
        ${NAMESPACE}JSONRPC::${NAMESPACE}JSONRPC
        )


install(TARGETS JSONRPCClient DESTINATION bin)
