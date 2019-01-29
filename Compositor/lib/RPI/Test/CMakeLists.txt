set(TEST RPICompositorTest)

message("Setting up ${TEST}")

find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}VirtualInput REQUIRED)

find_package(GLESv2 REQUIRED)
find_package(EGL REQUIRED)
find_package(PNG REQUIRED)

add_executable(${TEST} Test.cpp)

target_link_libraries(${TEST} 
    PRIVATE
        compositorclient
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        GLESV2::GLESV2
        EGL::EGL
        PNG::PNG)
