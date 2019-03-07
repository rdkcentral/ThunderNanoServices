
find_package(jsonrpc REQUIRED)

add_executable(jsonRPCExampleClient client.cpp)

set_target_properties(jsonRPCExampleClient PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )     

target_link_libraries(jsonRPCExampleClient
    PRIVATE
    jsonrpc::jsonrpc
        )

target_include_directories(jsonRPCExampleClient
    PRIVATE
)

target_compile_definitions(jsonRPCExampleClient
    PRIVATE
)

install(TARGETS jsonRPCExampleClient DESTINATION bin)
