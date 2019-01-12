set(COMPOSITOR_TEST_ARTIFACT RPICompositorTest)

message("Setting up ${COMPOSITOR_TEST_ARTIFACT}")

find_package(Core REQUIRED)
find_package(Plugins REQUIRED)
find_package(VirtualInput REQUIRED)

set(COMPOSITOR_TEST_LIBS
    ${CORE_LIBRARIES}
    ${PLUGINS_LIBRARIES}
    compositorclient
    GLESv2
    EGL
    png
    )

set(COMPOSITOR_TEST_SOURCES Test.cpp)

add_executable(${COMPOSITOR_TEST_ARTIFACT} ${COMPOSITOR_TEST_SOURCES})
target_link_libraries(${COMPOSITOR_TEST_ARTIFACT} ${COMPOSITOR_TEST_LIBS})

set_target_properties(${COMPOSITOR_TEST_ARTIFACT} PROPERTIES OUTPUT_NAME ${COMPOSITOR_TEST_ARTIFACT})
