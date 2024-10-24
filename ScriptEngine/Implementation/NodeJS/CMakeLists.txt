set(IMPLEMENTATION NodeJS)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

message(STATUS "Setting up ${IMPLEMENTATION} as implementation for ${MODULE_NAME}")

find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Messaging REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(NodeJS REQUIRED)

add_library(${IMPLEMENTATION} STATIC Implementation.cpp)

target_include_directories(${IMPLEMENTATION} PRIVATE ${NODEJS_INCLUDE_DIRS})

target_link_libraries(${IMPLEMENTATION}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging)

set_target_properties(${IMPLEMENTATION} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE)

add_library(common::implementation ALIAS ${IMPLEMENTATION})
