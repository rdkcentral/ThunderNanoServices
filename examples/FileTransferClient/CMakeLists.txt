set(PLUGIN_NAME FileTransferClient)

find_package(${NAMESPACE}Core REQUIRED)

add_executable(${PLUGIN_NAME} FileTransferClient.cpp)

set_target_properties(${PLUGIN_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )     

target_link_libraries(${PLUGIN_NAME}
    PRIVATE 
        ${NAMESPACE}Core::${NAMESPACE}Core)

target_include_directories(${PLUGIN_NAME} 
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core)

install(TARGETS ${PLUGIN_NAME} DESTINATION bin)