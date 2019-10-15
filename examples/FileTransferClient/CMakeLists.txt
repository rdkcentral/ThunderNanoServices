find_package(jsonrpc REQUIRED)

add_executable(FileTransferClient FileTransferClient.cpp)

set_target_properties(FileTransferClient PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )     

target_link_libraries(FileTransferClient 
        PRIVATE
        jsonrpc::jsonrpc
    )

target_include_directories(FileTransferClient 
    PRIVATE
)

install(TARGETS FileTransferClient DESTINATION bin)