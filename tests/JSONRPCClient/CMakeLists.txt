find_package(jsonrpc REQUIRED)

add_executable(JSONRPCClient JSONRPCClient.cpp)

set_target_properties(JSONRPCClient PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )     

target_link_libraries(JSONRPCClient 
        PRIVATE
        jsonrpc::jsonrpc
    )

target_include_directories(JSONRPCClient 
    PRIVATE
         "${PROJECT_SOURCE_DIR}/tests"
)

install(TARGETS JSONRPCClient DESTINATION bin)
